#include "shop_app.h"
#include "shop_chinese.h"
#include "ff.h"
#include "drivers.h"

// 优惠类型枚举（值与复选框索引对齐）
typedef enum {
    DISCOUNT_NONE = 0,
    DISCOUNT_FULL_REDUCTION,    // 满20减5
    DISCOUNT_PERCENT_OFF,       // 9折
    DISCOUNT_FULL_100_80PCT,    // 满100打8折
    DISCOUNT_FULL_200_50        // 满200减50
} discount_type_t;

// 键盘输入模式枚举（决定确认后执行什么行为）
// 定义在 shop_app.h 中，此处不再重复

int current_discount = DISCOUNT_NONE;
kb_input_mode_t current_kb_mode = KB_MODE_NONE;  // 当前键盘输入模式（外部可访问）
int coupon_remaining[5] = {-1, 10, 10, 10, 10};   // 每种优惠券剩余数量，-1表示无限(无优惠)

// 记录当前点击的是哪个商品，用于键盘确认时计算
static product_t * current_product = NULL;

// 引用在 ui.c 中定义的变量
extern lv_obj_t * cart_list;
extern lv_obj_t * input_ta;
extern lv_obj_t * num_kb;
extern lv_obj_t * label_full;

// --- A. 点击商品卡片的回调 ---

// 延时清空购物车的定时器回调（repeat_count=1，LVGL 自动删除）
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

void product_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        product_t * p = (product_t *)lv_event_get_user_data(e);
        if(p == NULL || input_ta == NULL || num_kb == NULL) return;

        current_product = p; // 存入静态变量
        current_kb_mode = KB_MODE_ADD_TO_CART;  // 设置输入模式

        // 动态设置文本框提示语（中文版）
        static char prompt_str[96];
        snprintf(prompt_str, sizeof(prompt_str), "%s%s%s%s%s", 
                 CN_QTY_PREFIX, p->name, CN_QTY_MID, p->unit, CN_QTY_SUFFIX);
        lv_textarea_set_placeholder_text(input_ta, prompt_str);
        
        // 显示输入界面
        show_input_ui();
    }
}

// ========== 输入界面显示/隐藏 ==========

/* 显示输入界面（键盘 + 文本框 + 遮罩） */
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

// ========== 拆分的辅助函数 ==========

/* 判断商品是否需要整数输入（按个卖的商品） */
static bool is_integer_product(const product_t * product)
{
    if(product == NULL) return false;
    
    return (strcmp(product->unit, CN_BOX) == 0 ||
            strcmp(product->unit, CN_PACK) == 0 ||
            strcmp(product->unit, CN_BOTTLE) == 0);
}

/* 验证并解析输入，返回是否有效 */
typedef enum {
    INPUT_OK = 0,               // 输入有效
    INPUT_ERR_DECIMAL,          // 整数商品输入了小数
    INPUT_ERR_NON_POSITIVE      // 数量不是正数（<= 0）
} input_error_t;

typedef struct {
    bool is_valid;
    float quantity;
    float total_price;
    char item_text[128];
    input_error_t error;        // 错误类型（is_valid=false 时有效）
} input_result_t;

static input_result_t validate_and_parse_input(const char * input_str, const product_t * product)
{
    input_result_t result = {0};
    
    if(input_str == NULL || strlen(input_str) == 0 || product == NULL) {
        return result;
    }
    
    // 检查整数商品不能有小数点
    if(is_integer_product(product) && strchr(input_str, '.') != NULL) {
        result.error = INPUT_ERR_DECIMAL;
        return result;
    }
    
    // 解析数量
    result.quantity = (float)atof(input_str);
    if(result.quantity <= 0) {
        result.error = INPUT_ERR_NON_POSITIVE;
        return result;
    }
    if(result.quantity > 9999) {
        result.quantity = 9999;
    }
    
    // 计算总价（price 存储单位为分，需除以100得到元）
    result.total_price = result.quantity * ((float)product->price / 100.0f);
    
    // 构建购物车项文本
    snprintf(result.item_text, sizeof(result.item_text), 
             "%s x%.1f %s = %.2f " CN_YUAN, 
             product->name, result.quantity, product->unit, result.total_price);
    
    result.is_valid = true;
    return result;
}

/* 隐藏输入界面并重置状态 */
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
    current_kb_mode = KB_MODE_NONE;  // 重置输入模式
}

// ========== 键盘确认后的模式处理函数 ==========

/* 处理"添加到购物车"模式的确认 */
static void kb_mode_add_to_cart_handler(const char * input_str)
{
    if(current_product == NULL || cart_list == NULL) {
        hide_input_ui();
        return;
    }

    // 验证并解析输入
    input_result_t result = validate_and_parse_input(input_str, current_product);
    
    if(!result.is_valid) {
        // 根据错误类型显示不同提示
        switch(result.error) {
            case INPUT_ERR_DECIMAL:
                shop_ui_show_msgbox(CN_ERROR, CN_INT_ONLY, NULL);
                break;
            case INPUT_ERR_NON_POSITIVE:
                shop_ui_show_msgbox(CN_ERROR, CN_QTY_POSITIVE, NULL);
                break;
            default:
                break;
        }
        hide_input_ui();
        led_blink_n(LED_RED, 70, 3);
        return;
    }

    // 添加到购物车
    shop_ui_add_cart_item(result.item_text, current_product->name);
    shop_ui_update_cart_total();
    hide_input_ui();
    // 黄灯短闪：添加成功
    led_blink(LED_YELLOW, 70);
}

/* 处理"修改数量"模式的确认 */
static void kb_mode_edit_quantity_handler(const char * input_str)
{
    if(current_product == NULL || cart_list == NULL) {
        hide_input_ui();
        return;
    }

    // 验证并解析输入
    input_result_t result = validate_and_parse_input(input_str, current_product);
    
    if(!result.is_valid) {
        switch(result.error) {
            case INPUT_ERR_DECIMAL:
                shop_ui_show_msgbox(CN_ERROR, CN_INT_ONLY, NULL);
                break;
            case INPUT_ERR_NON_POSITIVE:
                shop_ui_show_msgbox(CN_ERROR, CN_QTY_POSITIVE, NULL);
                break;
            default:
                break;
        }
        hide_input_ui();
        led_blink_n(LED_RED, 70, 3);
        return;
    }

    // 更新购物车中的商品项
    shop_ui_add_cart_item(result.item_text, current_product->name);
    shop_ui_update_cart_total();

    hide_input_ui();
}

// --- B. 键盘事件回调 (模式分发) ---
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

        // 根据当前输入模式分发到对应的处理函数
        switch(current_kb_mode) {
            case KB_MODE_ADD_TO_CART:
                kb_mode_add_to_cart_handler(input_str);
                break;
            case KB_MODE_EDIT_QUANTITY:
                kb_mode_edit_quantity_handler(input_str);
                break;

            case KB_MODE_NONE:
            default:
                hide_input_ui();
                break;
        }
    }
    else if(code == LV_EVENT_CANCEL) {
        hide_input_ui();
    }
}

// 异步LED闪烁回调函数（避免阻塞UI）
static transaction_t pending_tx;
static void async_tx_log_cb(lv_timer_t * timer) {
    tx_log_add(&pending_tx);
    lv_timer_del(timer);
}

static void async_led_blue_cb(lv_timer_t * timer) {
    led_blink_n(LED_BLUE, 60, 2);
    lv_timer_del(timer);
}

static void async_led_green_cb(lv_timer_t * timer) {
    led_blink(LED_GREEN, 100);
    lv_timer_del(timer);
}

static void async_led_green2_cb(lv_timer_t * timer) {
    led_blink(LED_GREEN, 100);
    lv_timer_del(timer);
}

// --- C. 结算按钮回调 (解析字符串求和) ---
void checkout_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(cart_list == NULL) return;

    float grand_total = 0.0f;
    uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);

    // 1. 检查购物车是否为空（child_cnt <= 1 表示只有标题行或空列表）
    if(child_cnt <= 1) {
        shop_ui_show_msgbox(CN_HINT, CN_CART_EMPTY, NULL);
        led_blink_n(LED_RED, 70, 3);
        return;
    }

    // 2. 遍历列表，解析 "=" 后的价格并累加（从 i=1 开始，跳过标题行）
    for(uint32_t i = 1; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cart_list, i);
        const char * text = lv_list_get_btn_text(cart_list, child);
        if(text) {
            const char * equal_sign = strchr(text, '=');
            if(equal_sign != NULL) {
                // 跳过 '=' 和空格，转换后面的数字
                grand_total += (float)atof(equal_sign + 1);
            }
        }
    }

    // 3. 总价为0时提示购物车为空
    if(grand_total == 0) {
        shop_ui_show_msgbox(CN_HINT, CN_CART_EMPTY, NULL);
        led_blink_n(LED_RED, 70, 3);
        return;
    }

		//对grand_total进行打折
		if (current_discount != DISCOUNT_NONE && coupon_remaining[current_discount] <= 0) {
		    shop_ui_show_msgbox(CN_HINT, CN_COUPON_USED_UP, NULL);
		    led_blink_n(LED_RED, 70, 3);
		    return;
		}

		float final_total = grand_total;
    const char * discount_desc = "";

    switch (current_discount) {
        case DISCOUNT_FULL_REDUCTION:   // 满20减5
            if (grand_total >= 20.0f) {
                final_total = grand_total - 5.0f;
                discount_desc = CN_DESC_DISC_FULL20;
            }
            break;
        case DISCOUNT_PERCENT_OFF:      // 9折
            final_total = grand_total * 0.9f;
            discount_desc = CN_DESC_DISC_90PCT;
            break;
        case DISCOUNT_FULL_100_80PCT:   // 满100打8折
            if (grand_total >= 100.0f) {
                final_total = grand_total * 0.8f;
                discount_desc = CN_DESC_DISC_FULL100_80PCT;
            }
            break;
        case DISCOUNT_FULL_200_50:      // 满200减50
            if (grand_total >= 200.0f) {
                final_total = grand_total - 50.0f;
                discount_desc = CN_DESC_DISC_FULL200_RED50;
            }
            break;
        case DISCOUNT_NONE:
        default:
            discount_desc = CN_DESC_DISC_NONE;
            break;
    }

    // 优惠已生效则消耗一张优惠券
    if (current_discount != DISCOUNT_NONE && discount_desc[0] != '\0') {
        coupon_remaining[current_discount]--;
        shop_ui_update_coupon_display();
            coupon_config_save();  // 持久化到SD卡
        if (coupon_remaining[current_discount] <= 0) {
            lv_obj_clear_state(discount_checkboxes[current_discount], LV_STATE_CHECKED);
            lv_obj_add_state(discount_checkboxes[DISCOUNT_NONE], LV_STATE_CHECKED);
            current_discount = DISCOUNT_NONE;
        }
    }

    // 防止折扣后金额为负数
    if (final_total < 0) final_total = 0;

    // 4. 构建交易记录对象并保存到 SD 卡
    transaction_t tx;
    memset(&tx, 0, sizeof(transaction_t));
    
    tx.total_before_discount = grand_total;
    tx.total_after_discount = final_total;
    tx.discount_type = (tx_discount_type_t)current_discount;
    tx.item_count = 0;

    // 遍历购物车，提取每个商品的明细（排除标题行）
    for(uint32_t i = 1; i < child_cnt && tx.item_count < MAX_TX_ITEMS_PER_TX; i++) {
        lv_obj_t * child = lv_obj_get_child(cart_list, i);
        const char * text = lv_list_get_btn_text(cart_list, child);
        
        if(text) {
            // 格式: "苹果 x2.0 kg = 16.00 元"
            // 从文字中提取商品名、数量、小计
            
            // 反向查找商品 ID
            uint8_t product_id = 0xFF;
            for(uint8_t j = 0; j < MAX_PRODUCTS; j++) {
                if(strncmp(text, shop_products[j].name, strlen(shop_products[j].name)) == 0) {
                    product_id = j;
                    break;
                }
            }

            if(product_id != 0xFF) {
                // 解析数量和小计
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

    // 保存交易记录到 SD 卡（使用定时器异步执行，避免阻塞UI）
    memcpy(&pending_tx, &tx, sizeof(transaction_t));
    lv_timer_t * tx_timer = lv_timer_create(async_tx_log_cb, 5, NULL);
    lv_timer_set_repeat_count(tx_timer, 1);

		// 5. 显示结算结果弹窗（先弹窗，再闪灯，避免迟滞感）
    shop_ui_show_checkout_result(&tx, grand_total, final_total, discount_desc);
    
    // 蓝灯闪2下：SD写入完成（100ms后执行）
    lv_timer_t * led_blue_timer = lv_timer_create(async_led_blue_cb, 100, NULL);
    lv_timer_set_repeat_count(led_blue_timer, 1);
    
    // 绿灯闪两下：结算成功（200ms和400ms后执行）
    lv_timer_t * led_green1_timer = lv_timer_create(async_led_green_cb, 200, NULL);
    lv_timer_set_repeat_count(led_green1_timer, 1);
    lv_timer_t * led_green2_timer = lv_timer_create(async_led_green2_cb, 400, NULL);
    lv_timer_set_repeat_count(led_green2_timer, 1);
    
    lv_timer_t * t = lv_timer_create(clear_cart_timer_cb, 500, NULL);
    lv_timer_set_repeat_count(t, 1);
}

// --- D. 清空回调 ---
void clear_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED && cart_list != NULL) {
        // 只删除商品项，不删除标题
        uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);
        for(int i = child_cnt - 1; i > 0; i--) {
            lv_obj_t * child = lv_obj_get_child(cart_list, i);
            lv_obj_del(child);
        }
    }
    shop_ui_update_cart_total();
}

// --- E. 空白区域点击隐藏键盘等 ---
void label_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
				lv_obj_add_flag(label_full, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
				lv_obj_add_flag(input_ta, LV_OBJ_FLAG_HIDDEN);
				lv_obj_move_background(label_full);
    }
}

/* ========== 优惠复选框互斥回调 ========== */
void discount_cb_event_cb(lv_event_t * e) {
    lv_obj_t * cb = lv_event_get_target(e);
    uint32_t idx = (uint32_t)lv_event_get_user_data(e);
    lv_obj_t * parent = lv_obj_get_parent(cb);

    if (lv_obj_has_state(cb, LV_STATE_CHECKED)) {
        // 优惠券已用完则不允许选中
        if (idx != DISCOUNT_NONE && coupon_remaining[idx] <= 0) {
            lv_obj_clear_state(cb, LV_STATE_CHECKED);
            lv_obj_add_state(discount_checkboxes[DISCOUNT_NONE], LV_STATE_CHECKED);
            current_discount = DISCOUNT_NONE;
            shop_ui_show_msgbox(CN_HINT, CN_COUPON_USED_UP, NULL);
            return;
        }
        // 清除同一容器内其他复选框的选中状态
        uint32_t child_cnt = lv_obj_get_child_cnt(parent);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(parent, i);
            if (child != cb && lv_obj_check_type(child, &lv_checkbox_class)) {
                lv_obj_clear_state(child, LV_STATE_CHECKED);
            }
        }
        // 更新全局选中状态
        current_discount = (discount_type_t)idx;
    }
    else {
        // 不允许取消选中，强制重新勾选
        lv_obj_add_state(cb, LV_STATE_CHECKED);
    }
}

// 购物车操作菜单辅助函数前向声明
static void cart_action_delete_cb(lv_event_t * e);
static void cart_action_edit_cb(lv_event_t * e);

// 购物车按钮项点击回调：点击商品项弹出操作菜单（删除/修改数量）
void cart_list_btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
        if(btn == NULL) return;

        // 获取当前商品项的文字
        const char * text = lv_list_get_btn_text(cart_list, btn);
        if(text == NULL) return;

        /* 创建操作选择弹窗，获取删除和修改按钮并绑定回调 */
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

/* 删除商品按钮回调 */
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

/* 修改数量按钮回调 */
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

// ==================== 交易历史面板 ====================

/* 交易记录按钮回调 */
void history_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        show_history_panel();
    }
}

// ==================== 优惠券配置持久化 ====================

/* 从 SD 卡加载优惠券数量 */
void coupon_config_load(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    char buf[64];

    res = f_open(&file, COUPON_CONFIG_FILE, FA_READ);
    if(res != FR_OK) return;  // 文件不存在是正常的（首次使用）

    FSIZE_t file_size = f_size(&file);
    if(file_size >= sizeof(buf)) {
        f_close(&file);
        return;
    }

    res = f_read(&file, buf, (UINT)file_size, &bytes_read);
    f_close(&file);
    if(res != FR_OK || bytes_read == 0) return;

    buf[bytes_read] = '\0';

    // 解析格式: -1,10,10,10,10
    char * p = buf;
    for (int i = 0; i < 5; i++) {
        coupon_remaining[i] = (int)strtol(p, &p, 10);
        if (*p == ',') p++;
    }
}

/* 保存优惠券数量到 SD 卡 */
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

// ==================== 价格配置持久化 ====================

/* 从 SD 卡加载商品价格 */
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

    // 解析格式: 8,6,10,3,3,40
    char * p = buf;
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        uint32_t price = (uint32_t)strtol(p, &p, 10);
        if (price > 0 && price < 99999) {
            shop_products[i].price = price;
        }
        if (*p == ',') p++;
    }
}

/* 保存商品价格到 SD 卡 */
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

// ==================== "再来一单" 功能 ====================

bool reorder_pending = false;  // 再来一单待处理标记

/* 淡出关闭弹窗 → 直接跳转购物界面 */
static void reorder_close_ready_cb(lv_anim_t * a)
{
    lv_obj_del((lv_obj_t *)a->var);   // 删除弹窗遮罩
    show_shop_screen();                // 直接进购物界面
}

/* "再来一单"按钮回调：淡出弹窗 → 设置标记 → 回主页 → 自动进购物 */
void reorder_btn_cb(lv_event_t * e)
{
    lv_obj_t * overlay = (lv_obj_t *)lv_event_get_user_data(e);
    if(overlay == NULL || current_detail_tx == NULL) return;

    reorder_pending = true;

    // 淡出动画关闭弹窗（与关闭按钮相同）
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, overlay);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 100);
    lv_anim_set_ready_cb(&a, reorder_close_ready_cb);
    lv_anim_start(&a);
}
