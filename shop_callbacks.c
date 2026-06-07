#include "shop_app.h"
#include "shop_chinese.h"
#include "ff.h"
#include "drivers.h"

// 优惠类型 
typedef enum {
    DISCOUNT_NONE = 0,
    DISCOUNT_FULL_REDUCTION,
    DISCOUNT_PERCENT_OFF,
    DISCOUNT_FULL_100_80PCT,
    DISCOUNT_FULL_200_50
} discount_type_t;

int current_discount = DISCOUNT_NONE;
kb_input_mode_t current_kb_mode = KB_MODE_NONE;
int coupon_remaining[5] = {-1, 10, 10, 10, 10};

static product_t * current_product = NULL;

extern lv_obj_t * cart_list;
extern lv_obj_t * input_ta;
extern lv_obj_t * num_kb;
extern lv_obj_t * label_full;

// 定时器：清空购物车
static void clear_cart_timer_cb(lv_timer_t * timer)
{
    if(cart_list != NULL) {
        uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);
        for(int i = child_cnt - 1; i > 0; i--) {
            lv_obj_del(lv_obj_get_child(cart_list, i));
        }
    }
    shop_ui_update_cart_total();
}

// 商品点击事件
void product_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        product_t * p = (product_t *)lv_event_get_user_data(e);
        if(p == NULL || input_ta == NULL || num_kb == NULL) return;

        current_product = p;
        current_kb_mode = KB_MODE_ADD_TO_CART;

        static char prompt_str[96];
        snprintf(prompt_str, sizeof(prompt_str), "%s%s%s%s%s",
                 CN_QTY_PREFIX, p->name, CN_QTY_MID, p->unit, CN_QTY_SUFFIX);
        lv_textarea_set_placeholder_text(input_ta, prompt_str);
        show_input_ui();
    }
}

// 显示输入面板
void show_input_ui(void)
{
    lv_keyboard_set_textarea(num_kb, input_ta);
    lv_obj_clear_flag(input_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(label_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(label_full);
    lv_obj_move_foreground(num_kb);
    lv_obj_move_foreground(input_ta);
}

// 判断商品是否必须整数个
static int is_integer_product(const product_t * product)
{
    if(product == NULL) return 0;

    if(strcmp(product->unit, CN_BOX) == 0)    return 1;
    if(strcmp(product->unit, CN_PACK) == 0)   return 1;
    if(strcmp(product->unit, CN_BOTTLE) == 0) return 1;
    return 0;
}

// 输入的验证结果
typedef struct {
    int is_valid;
    float quantity;
    float total_price;
    char item_text[128];
    int error_type;  // 0=ok, 1=整数商品输入了小数, 2=数量不是正数
} input_result_t;

// 解析用户输入
static input_result_t validate_and_parse_input(const char * input_str, const product_t * product)
{
    input_result_t result;
    memset(&result, 0, sizeof(result));

    if(input_str == NULL || strlen(input_str) == 0 || product == NULL) {
        return result;
    }

    // 整数商品不能有小数点
    if (is_integer_product(product) && strchr(input_str, '.') != NULL) {
        result.error_type = 1;
        return result;
    }

    result.quantity = (float)atof(input_str);
    if (result.quantity <= 0) {
        result.error_type = 2;
        return result;
    }
    if (result.quantity > 9999) result.quantity = 9999;

    result.total_price = result.quantity * ((float)product->price / 100.0f);

    snprintf(result.item_text, sizeof(result.item_text),
             "%s x%.1f %s = %.2f " CN_YUAN,
             product->name, result.quantity, product->unit, result.total_price);

    result.is_valid = 1;
    return result;
}

// 隐藏输入面板
void hide_input_ui(void)
{
    if(input_ta != NULL) {
        lv_obj_add_flag(input_ta, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(input_ta, "");
    }
    if(num_kb != NULL) {
        lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
    }
    if(label_full != NULL) {
        lv_obj_add_flag(label_full, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(label_full);
    }
    current_kb_mode = KB_MODE_NONE;
}

// 键盘确定键加入购物车
static void kb_mode_add_to_cart_handler(const char * input_str)
{
    if(current_product == NULL || cart_list == NULL) {
        hide_input_ui();
        return;
    }

    input_result_t result = validate_and_parse_input(input_str, current_product);

    if(!result.is_valid) {
        if(result.error_type == 1) {
            shop_ui_show_msgbox(CN_ERROR, CN_INT_ONLY, NULL);
        } else if(result.error_type == 2) {
            shop_ui_show_msgbox(CN_ERROR, CN_QTY_POSITIVE, NULL);
        }
        hide_input_ui();
        led_blink_n(LED_RED, 70, 3);
        return;
    }

    shop_ui_add_cart_item(result.item_text, current_product->name);
    shop_ui_update_cart_total();
    hide_input_ui();
    led_blink(LED_YELLOW, 70);
}

// 键盘确定键修改数量
static void kb_mode_edit_quantity_handler(const char * input_str)
{
    if(current_product == NULL || cart_list == NULL) {
        hide_input_ui();
        return;
    }

    input_result_t result = validate_and_parse_input(input_str, current_product);

    if(!result.is_valid) {
        if(result.error_type == 1) {
            shop_ui_show_msgbox(CN_ERROR, CN_INT_ONLY, NULL);
        } else if(result.error_type == 2) {
            shop_ui_show_msgbox(CN_ERROR, CN_QTY_POSITIVE, NULL);
        }
        hide_input_ui();
        led_blink_n(LED_RED, 70, 3);
        return;
    }

    shop_ui_add_cart_item(result.item_text, current_product->name);
    shop_ui_update_cart_total();

    hide_input_ui();
}

// 数字键盘事件
void kb_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_READY) {
        if(input_ta == NULL) return;

        const char * input_str = lv_textarea_get_text(input_ta);
        if(input_str == NULL || strlen(input_str) == 0) {
            hide_input_ui();
            return;
        }

        if(current_kb_mode == KB_MODE_ADD_TO_CART) {
            kb_mode_add_to_cart_handler(input_str);
        } else if(current_kb_mode == KB_MODE_EDIT_QUANTITY) {
            kb_mode_edit_quantity_handler(input_str);
        } else {
            hide_input_ui();
        }
    }
    else if(code == LV_EVENT_CANCEL) {
        hide_input_ui();
    }
}

// 结账相关

static transaction_t pending_tx;

static void async_tx_log_cb(lv_timer_t * timer) {
    tx_log_add(&pending_tx);
    lv_timer_del(timer);
}

// LED灯效任务
typedef struct {
    uint32_t pin;
    uint16_t ms;
    uint8_t  count;
} led_task_t;

static const led_task_t led_tasks[] = {
    {LED_BLUE,  60,  2},
    {LED_GREEN, 100, 0},
    {LED_GREEN, 100, 0},
};

static void async_led_cb(lv_timer_t * timer)
{
    int idx = (int)(uintptr_t)timer->user_data;
    const led_task_t * t = &led_tasks[idx];
    if (t->count > 0) led_blink_n(t->pin, t->ms, t->count);
    else              led_blink(t->pin, t->ms);
    lv_timer_del(timer);
}

// 结账按钮
void checkout_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(cart_list == NULL) return;

    float grand_total = 0.0f;
    uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);

    // 购物车为空报错
    if (child_cnt <= 1) {
        shop_ui_show_msgbox(CN_HINT, CN_CART_EMPTY, NULL);
        led_blink_n(LED_RED, 70, 3);
        return;
    }

    // 先算出折前总价
    for (uint32_t i = 1; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cart_list, i);
        const char * text = lv_list_get_btn_text(cart_list, child);
        if (text) {
            const char * equal_sign = strchr(text, '=');
            if (equal_sign != NULL)
                grand_total += (float)atof(equal_sign + 1);
        }
    }

    if (grand_total == 0) {
        shop_ui_show_msgbox(CN_HINT, CN_CART_EMPTY, NULL);
        led_blink_n(LED_RED, 70, 3);
        return;
    }

    // 检查优惠券还能不能用
    if (current_discount != DISCOUNT_NONE && coupon_remaining[current_discount] <= 0) {
        shop_ui_show_msgbox(CN_HINT, CN_COUPON_USED_UP, NULL);
        led_blink_n(LED_RED, 70, 3);
        return;
    }

    // 根据当前选中优惠计算折后价
    float final_total = grand_total;
    const char * discount_desc = "";

    if (current_discount == DISCOUNT_FULL_REDUCTION) {
        if (grand_total >= 20.0f) {
            final_total = grand_total - 5.0f;
            discount_desc = CN_DESC_DISC_FULL20;
        }
    } else if (current_discount == DISCOUNT_PERCENT_OFF) {
        final_total = grand_total * 0.9f;
        discount_desc = CN_DESC_DISC_90PCT;
    } else if (current_discount == DISCOUNT_FULL_100_80PCT) {
        if (grand_total >= 100.0f) {
            final_total = grand_total * 0.8f;
            discount_desc = CN_DESC_DISC_FULL100_80PCT;
        }
    } else if (current_discount == DISCOUNT_FULL_200_50) {
        if (grand_total >= 200.0f) {
            final_total = grand_total - 50.0f;
            discount_desc = CN_DESC_DISC_FULL200_RED50;
        }
    } else {
        discount_desc = CN_DESC_DISC_NONE;
    }

    // 如果用了优惠券，减一张
    if (current_discount != DISCOUNT_NONE && discount_desc[0] != '\0') {
        coupon_remaining[current_discount]--;
        shop_ui_update_coupon_display();
        coupon_config_save();
        if (coupon_remaining[current_discount] <= 0) {
            lv_obj_clear_state(discount_checkboxes[current_discount], LV_STATE_CHECKED);
            lv_obj_add_state(discount_checkboxes[DISCOUNT_NONE], LV_STATE_CHECKED);
            current_discount = DISCOUNT_NONE;
        }
    }

    if (final_total < 0) final_total = 0;

    // 构建交易记录
    transaction_t tx;
    memset(&tx, 0, sizeof(transaction_t));
    tx.total_before_discount = grand_total;
    tx.total_after_discount = final_total;
    tx.discount_type = (tx_discount_type_t)current_discount;
    tx.item_count = 0;

    for (uint32_t i = 1; i < child_cnt && tx.item_count < MAX_TX_ITEMS_PER_TX; i++) {
        lv_obj_t * child = lv_obj_get_child(cart_list, i);
        const char * text = lv_list_get_btn_text(cart_list, child);
        if (text) {
            uint8_t product_id = 0xFF;
            for (uint8_t j = 0; j < MAX_PRODUCTS; j++) {
                if(strncmp(text, shop_products[j].name, strlen(shop_products[j].name)) == 0) {
                    product_id = j;
                    break;
                }
            }

            if (product_id != 0xFF) {
                const char * x_pos = strchr(text, 'x');
                const char * eq_pos = strchr(text, '=');

                if(x_pos && eq_pos) {
                    float quantity = (float)atof(x_pos + 1);
                    float subtotal = (float)atof(eq_pos + 1);

                    tx.items[tx.item_count].product_id = product_id;
                    tx.items[tx.item_count].quantity = quantity;
                    tx.items[tx.item_count].subtotal = subtotal;
                    tx.item_count++;
                }
            }
        }
    }

    // 异步保存（定时器延迟一下，避免卡UI）
    memcpy(&pending_tx, &tx, sizeof(transaction_t));
    lv_timer_t * tx_timer = lv_timer_create(async_tx_log_cb, 5, NULL);
    lv_timer_set_repeat_count(tx_timer, 1);

    // 弹结算结果窗
    shop_ui_show_checkout_result(&tx, grand_total, final_total, discount_desc);

    // 结算成功灯效
    lv_timer_t * t = lv_timer_create(async_led_cb, 100, (void *)0);
    lv_timer_set_repeat_count(t, 1);
    t = lv_timer_create(async_led_cb, 200, (void *)1);
    lv_timer_set_repeat_count(t, 1);
    t = lv_timer_create(async_led_cb, 400, (void *)2);
    lv_timer_set_repeat_count(t, 1);

    // 延迟清空购物车
    t = lv_timer_create(clear_cart_timer_cb, 500, NULL);
    lv_timer_set_repeat_count(t, 1);
}

// 清空购物车
void clear_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED && cart_list != NULL) {
        uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);
        for(int i = child_cnt - 1; i > 0; i--) {
            lv_obj_t * child = lv_obj_get_child(cart_list, i);
            lv_obj_del(child);
        }
    }
    shop_ui_update_cart_total();
}

// 遮罩点击
void label_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        hide_input_ui();
        hide_password_ui();
    }
}

// 折扣复选框
void discount_cb_event_cb(lv_event_t * e) {
    lv_obj_t * cb = lv_event_get_target(e);
    uint32_t idx = (uint32_t)lv_event_get_user_data(e);
    lv_obj_t * parent = lv_obj_get_parent(cb);

    if (lv_obj_has_state(cb, LV_STATE_CHECKED)) {
        // 优惠券用完了不能选
        if (idx != DISCOUNT_NONE && coupon_remaining[idx] <= 0) {
            lv_obj_clear_state(cb, LV_STATE_CHECKED);
            lv_obj_add_state(discount_checkboxes[DISCOUNT_NONE], LV_STATE_CHECKED);
            current_discount = DISCOUNT_NONE;
            shop_ui_show_msgbox(CN_HINT, CN_COUPON_USED_UP, NULL);
            return;
        }
        // 清除其他复选框
        uint32_t child_cnt = lv_obj_get_child_cnt(parent);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(parent, i);
            if (child != cb && lv_obj_check_type(child, &lv_checkbox_class)) {
                lv_obj_clear_state(child, LV_STATE_CHECKED);
            }
        }
        current_discount = (discount_type_t)idx;
    }
    else {
        // 不允许自己取消（必须始终有一项被选中）
        lv_obj_add_state(cb, LV_STATE_CHECKED);
    }
}

static void cart_action_delete_cb(lv_event_t * e);
static void cart_action_edit_cb(lv_event_t * e);

// 购物车条目点击
void cart_list_btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
        if(btn == NULL) return;

        const char * text = lv_list_get_btn_text(cart_list, btn);
        if(text == NULL) return;

        shop_ui_show_cart_action_menu();
        lv_obj_t * del_btn  = shop_ui_get_cart_delete_btn();
        lv_obj_t * edit_btn = shop_ui_get_cart_edit_btn();

        if(del_btn != NULL) {
            lv_obj_add_event_cb(del_btn, cart_action_delete_cb, LV_EVENT_CLICKED, btn);
        }
        if(edit_btn != NULL) {
            lv_obj_add_event_cb(edit_btn, cart_action_edit_cb, LV_EVENT_CLICKED, btn);
        }
    }
}

// 删除按钮回调
static void cart_action_delete_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t * list_btn = (lv_obj_t *)lv_event_get_user_data(e);
    if(list_btn) {
        lv_obj_del(list_btn);
        shop_ui_update_cart_total();
    }
    shop_ui_close_cart_menu();
}

// 修改数量按钮回调
static void cart_action_edit_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_obj_t * list_btn = (lv_obj_t *)lv_event_get_user_data(e);
    if(list_btn) {
        const char * text = lv_list_get_btn_text(cart_list, list_btn);
        if(text) {
            product_t * target_product = NULL;
            for(uint8_t j = 0; j < MAX_PRODUCTS; j++) {
                if(strncmp(text, shop_products[j].name, strlen(shop_products[j].name)) == 0) {
                    target_product = &shop_products[j];
                    break;
                }
            }
            if(target_product) {
                current_product = target_product;
                current_kb_mode = KB_MODE_EDIT_QUANTITY;
                static char prompt_str[96];
                snprintf(prompt_str, sizeof(prompt_str), "%s%s%s",
                         CN_NEW_QTY_PREFIX, target_product->name, CN_NEW_QTY_MID);
                lv_textarea_set_placeholder_text(input_ta, prompt_str);
                show_input_ui();
            }
        }
    }
    shop_ui_close_cart_menu();
}

// 交易记录按钮回调
void history_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        show_history_panel();
    }
}

// 配置文件读写

// 从 SD 卡加载优惠券数量
void coupon_config_load(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    char buf[64];

    res = f_open(&file, COUPON_CONFIG_FILE, FA_READ);
    if(res != FR_OK) return;

    FSIZE_t file_size = f_size(&file);
    if(file_size >= sizeof(buf)) {
        f_close(&file);
        return;
    }

    res = f_read(&file, buf, (UINT)file_size, &bytes_read);
    f_close(&file);
    if(res != FR_OK || bytes_read == 0) return;

    buf[bytes_read] = '\0';

    // 格式: -1,10,10,10,10
    char * p = buf;
    for (int i = 0; i < 5; i++) {
        coupon_remaining[i] = (int)strtol(p, &p, 10);
        if (*p == ',') p++;
    }
}

// 保存优惠券数量到 SD 卡
void coupon_config_save(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_written;
    char buf[64];

    snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d\r\n",
             coupon_remaining[0], coupon_remaining[1],
             coupon_remaining[2], coupon_remaining[3],
             coupon_remaining[4]);

    res = f_open(&file, COUPON_CONFIG_FILE, FA_WRITE | FA_CREATE_ALWAYS);
    if(res != FR_OK) return;

    f_write(&file, buf, strlen(buf), &bytes_written);
    f_close(&file);
}

// 从 SD 卡加载商品价格
void price_config_load(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    char buf[64];

    res = f_open(&file, PRICE_CONFIG_FILE, FA_READ);
    if(res != FR_OK) return;

    FSIZE_t file_size = f_size(&file);
    if(file_size >= sizeof(buf)) {
        f_close(&file);
        return;
    }

    res = f_read(&file, buf, (UINT)file_size, &bytes_read);
    f_close(&file);
    if(res != FR_OK || bytes_read == 0) return;

    buf[bytes_read] = '\0';

    // 格式: 800,600,1000,300,300,4000
    char * p = buf;
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        uint32_t price = (uint32_t)strtol(p, &p, 10);
        if (price > 0 && price < 99999) {
            shop_products[i].price = price;
        }
        if (*p == ',') p++;
    }
}

// 保存商品价格到 SD 卡
void price_config_save(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_written;
    char buf[64];

    snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d,%d\r\n",
             (int)shop_products[0].price, (int)shop_products[1].price,
             (int)shop_products[2].price, (int)shop_products[3].price,
             (int)shop_products[4].price, (int)shop_products[5].price);

    res = f_open(&file, PRICE_CONFIG_FILE, FA_WRITE | FA_CREATE_ALWAYS);
    if(res != FR_OK) return;

    f_write(&file, buf, strlen(buf), &bytes_written);
    f_close(&file);
}

// 页面跳转回调

void shop_btn_cb(lv_event_t * e)
{
    (void)e;
    show_shop_screen();
}

void history_btn_home_cb(lv_event_t * e)
{
    (void)e;
    show_history_panel();
}

void back_btn_cb(lv_event_t * e)
{
    (void)e;
    show_home_screen();
}

void shop_back_btn_cb(lv_event_t * e)
{
    (void)e;
    show_password_ui();
}

// 密码键盘
void password_kb_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        if (password_input_ta == NULL) return;

        const char * input_str = lv_textarea_get_text(password_input_ta);
        if (input_str == NULL || strlen(input_str) == 0) {
            hide_password_ui();
            return;
        }

        if (strcmp(input_str, RESET_PASSWORD) == 0) {
            // 重置密码：静默恢复出厂设置
            tx_log_clear();
            for (int i = 0; i < MAX_PRODUCTS; i++) {
                shop_products[i].price = default_prices[i];
            }
            for (int i = 1; i <= 4; i++) {
                coupon_remaining[i] = 10;
            }
            coupon_config_save();
            price_config_save();
            f_unlink(COUPON_CONFIG_FILE);
            f_unlink(PRICE_CONFIG_FILE);
            led_blink_n(LED_YELLOW, 50, 3);     // 快闪3下黄灯确认
            hide_password_ui();
            show_home_screen();
        } else if (strcmp(input_str, ADMIN_PASSWORD) == 0) {
            hide_password_ui();
            show_home_screen();
        } else {
            shop_ui_show_msgbox(CN_ERROR, CN_PASSWORD_WRONG, NULL);
            lv_textarea_set_text(password_input_ta, "");
            led_blink_n(LED_RED, 70, 3);
        }
    }
    else if (code == LV_EVENT_CANCEL) {
        hide_password_ui();
    }
}

// 优惠券管理按钮回调
void coupon_mgmt_btn_home_cb(lv_event_t * e)
{
    (void)e;
    show_coupon_mgmt_screen();
}

// 优惠券管理 +/- 按钮
void coupon_mgmt_plus_minus_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_obj_t * btn = lv_event_get_target(e);
    int coupon_idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (coupon_idx < 1 || coupon_idx > 4) return;

    lv_obj_t * lbl = lv_obj_get_child(btn, 0);
    if (lbl == NULL) return;
    const char * txt = lv_label_get_text(lbl);
    if (txt == NULL) return;

    if (txt[0] == '+') {
        if (coupon_remaining[coupon_idx] < 999) {
            coupon_remaining[coupon_idx]++;
        }
    } else if (txt[0] == '-') {
        if (coupon_remaining[coupon_idx] > 0) {
            coupon_remaining[coupon_idx]--;
        }
    }

    shop_ui_update_coupon_mgmt_display();
    shop_ui_update_coupon_display();
    coupon_config_save();
}

// 优惠券重置
void coupon_mgmt_reset_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    for (int i = 1; i <= 4; i++) {
        coupon_remaining[i] = 10;
    }

    shop_ui_update_coupon_mgmt_display();
    shop_ui_update_coupon_display();
    coupon_config_save();
}

void price_mgmt_btn_home_cb(lv_event_t * e)
{
    (void)e;
    show_price_mgmt_screen();
}

// 价格管理 +/- 按钮
void price_mgmt_plus_minus_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_obj_t * btn = lv_event_get_target(e);
    int prod_idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (prod_idx < 0 || prod_idx >= MAX_PRODUCTS) return;

    lv_obj_t * lbl = lv_obj_get_child(btn, 0);
    if (lbl == NULL) return;
    const char * txt = lv_label_get_text(lbl);
    if (txt == NULL) return;

    if (txt[0] == '+') {
        if (shop_products[prod_idx].price < 99999) {
            shop_products[prod_idx].price += 50;
        }
    } else if (txt[0] == '-') {
        if (shop_products[prod_idx].price > 50) {
            shop_products[prod_idx].price -= 50;
        }
    }

    shop_ui_update_price_mgmt_display();
    shop_ui_update_shop_prices();
    price_config_save();
}

// 价格重置为默认
void price_mgmt_reset_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    for (int i = 0; i < MAX_PRODUCTS; i++) {
        shop_products[i].price = default_prices[i];
    }

    shop_ui_update_price_mgmt_display();
    shop_ui_update_shop_prices();
    price_config_save();
}

// 价格标签点击
void price_label_click_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int prod_idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (prod_idx < 0 || prod_idx >= MAX_PRODUCTS) return;
    show_price_input_ui(prod_idx);
}

// 价格键盘
void price_kb_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY) {
        if (price_input_ta == NULL || price_edit_idx < 0) return;

        const char * input_str = lv_textarea_get_text(price_input_ta);
        if (input_str == NULL || strlen(input_str) == 0) {
            hide_price_input_ui();
            return;
        }

        float new_price_float = atof(input_str);
        int new_price = (int)(new_price_float * 100.0f + 0.5f);
        if (new_price <= 0) {
            shop_ui_show_msgbox(CN_ERROR, CN_PRICE_INVALID, NULL);
            led_blink_n(LED_RED, 70, 3);
            hide_price_input_ui();
            return;
        }
        if (new_price > 99999) new_price = 99999;

        shop_products[price_edit_idx].price = (uint32_t)new_price;
        shop_ui_update_price_mgmt_display();
        shop_ui_update_shop_prices();
        price_config_save();

        hide_price_input_ui();
    }
    else if (code == LV_EVENT_CANCEL) {
        hide_price_input_ui();
    }
}

void price_overlay_click_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    hide_price_input_ui();
}

// 清空交易记录
void hist_clear_cb(lv_event_t * e)
{
    (void)e;
    tx_log_clear();
    shop_ui_refresh_history_list();
}

// 历史记录条目点击
void hist_list_item_cb(lv_event_t * e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if(idx < 0 || idx >= (int)tx_log.count) return;
    shop_ui_show_tx_detail(&tx_log.records[idx], (uint8_t)idx);
}

// 统计按钮回调
void detail_statistics_btn_cb(lv_event_t * e)
{
    (void)e;
    show_statistics_screen();
}

// 统计页面返回按钮
void statistics_back_btn_cb(lv_event_t * e)
{
    (void)e;
    show_history_panel_back();
}
