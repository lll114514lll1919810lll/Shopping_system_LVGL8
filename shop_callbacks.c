#include "shop_app.h"
#include "shop_chinese.h"

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

// 获取当前选中的优惠（供结算使用）
discount_type_t get_current_discount(void);

static discount_type_t current_discount = DISCOUNT_NONE;
kb_input_mode_t current_kb_mode = KB_MODE_NONE;  // 当前键盘输入模式（外部可访问）

discount_type_t get_current_discount(void) {
    return current_discount;
}

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
    
    // 计算总价
    result.total_price = result.quantity * (float)product->price;
    
    // 构建购物车项文本
    snprintf(result.item_text, sizeof(result.item_text), 
             "%s x%.1f %s = %.2f " CN_YUAN, 
             product->name, result.quantity, product->unit, result.total_price);
    
    result.is_valid = true;
    return result;
}

/* 在购物车中查找指定商品的按钮项 */
static lv_obj_t * find_cart_item_by_name(const char * product_name)
{
    if(cart_list == NULL || product_name == NULL) return NULL;
    
    uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cart_list, i);
        const char * text = lv_list_get_btn_text(cart_list, child);
        
        if(text && strncmp(text, product_name, strlen(product_name)) == 0) {
            return child;
        }
    }
    return NULL;
}

/* 添加或更新购物车项 */
static void add_or_update_cart_item(const char * item_text, const char * product_name)
{
    if(cart_list == NULL || item_text == NULL || product_name == NULL) return;
    
    // 查找是否已存在
    lv_obj_t * existing_btn = find_cart_item_by_name(product_name);
    if(existing_btn != NULL) {
        lv_obj_del(existing_btn);
    }
    
    // 添加新项
    lv_obj_t * btn = lv_list_add_btn(cart_list, LV_SYMBOL_OK, item_text);
    lv_obj_add_event_cb(btn, cart_list_btn_event_cb, LV_EVENT_CLICKED, NULL);
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

/* 显示错误提示 */
static void show_error_message(const char * title, const char * message)
{
    lv_obj_t * mbox = lv_msgbox_create(NULL, title, message, NULL, true);
    lv_obj_center(mbox);
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
                show_error_message(CN_ERROR, CN_INT_ONLY);
                break;
            case INPUT_ERR_NON_POSITIVE:
                show_error_message(CN_ERROR, CN_QTY_POSITIVE);
                break;
            default:
                break;
        }
        hide_input_ui();
        return;
    }

    // 添加到购物车
    add_or_update_cart_item(result.item_text, current_product->name);
    hide_input_ui();
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
                show_error_message(CN_ERROR, CN_INT_ONLY);
                break;
            case INPUT_ERR_NON_POSITIVE:
                show_error_message(CN_ERROR, CN_QTY_POSITIVE);
                break;
            default:
                break;
        }
        hide_input_ui();
        return;
    }

    // 根据商品名查找购物车中的项
    lv_obj_t * existing_btn = find_cart_item_by_name(current_product->name);
    if(existing_btn != NULL) {
        lv_obj_del(existing_btn);
    }

    // 添加新的购物车项
    lv_obj_t * btn = lv_list_add_btn(cart_list, LV_SYMBOL_OK, result.item_text);
    lv_obj_add_event_cb(btn, cart_list_btn_event_cb, LV_EVENT_CLICKED, NULL);

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
            
            // 可扩展更多模式处理：
            // case KB_MODE_SET_PRICE:
            //     kb_mode_set_price_handler(input_str);
            //     break;
            
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

// --- C. 结算按钮回调 (解析字符串求和) ---
void checkout_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(cart_list == NULL) return;

    float grand_total = 0.0f;
    uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);

    // 1. 检查购物车是否为空（child_cnt <= 1 表示只有标题行或空列表）
    if(child_cnt <= 1) {
        lv_obj_t * mbox = lv_msgbox_create(NULL, CN_HINT, CN_CART_EMPTY, NULL, true);
        lv_obj_t * title_lbl = lv_msgbox_get_title(mbox);
        if(title_lbl) lv_obj_set_style_text_font(title_lbl, &ziti_title, 0);
        lv_obj_center(mbox);
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
        lv_obj_t * mbox = lv_msgbox_create(NULL, CN_HINT, CN_CART_EMPTY, NULL, true);
        lv_obj_t * title_lbl = lv_msgbox_get_title(mbox);
        if(title_lbl) lv_obj_set_style_text_font(title_lbl, &ziti_title, 0);
        lv_obj_center(mbox);
        return;
    }

		//对grand_total进行打折
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

    // 保存交易记录到 RAM + SD 卡
    tx_log_add(&tx);

		// 5. 准备结果字符串 (包含商品明细 + 金额)
    static char result_buf[512];
    int offset = 0;

    // 列出每个商品的明细
    for(uint8_t k = 0; k < tx.item_count; k++) {
        uint8_t pid = tx.items[k].product_id;
        if(pid < MAX_PRODUCTS) {
            offset += snprintf(result_buf + offset, sizeof(result_buf) - offset,
                               "%s x%.1f %s = %.2f " CN_YUAN "\n",
                               shop_products[pid].name,
                               tx.items[k].quantity,
                               shop_products[pid].unit,
                               tx.items[k].subtotal);
        }
    }

    // 分隔线
    offset += snprintf(result_buf + offset, sizeof(result_buf) - offset,
                       "------------------\n");

    // 金额（有折扣时显示折前折后）
    if (grand_total != final_total) {
        snprintf(result_buf + offset, sizeof(result_buf) - offset,
                 CN_BEFORE_DISC ": %.2f " CN_YUAN "\n" CN_AFTER_DISC ": %.2f " CN_YUAN "%s",
                 grand_total, final_total, discount_desc);
    } else {
        snprintf(result_buf + offset, sizeof(result_buf) - offset,
                 CN_TOTAL ": %.2f " CN_YUAN, final_total);
    }

    // 6. 创建弹窗
    lv_obj_t * mbox = lv_msgbox_create(NULL, CN_SUCCESS, result_buf, NULL, true);
    if(mbox) {
        lv_obj_center(mbox);
        lv_obj_t * mbox_txt = lv_msgbox_get_text(mbox);
        if(mbox_txt) {
            lv_obj_set_style_text_color(mbox_txt, lv_palette_main(LV_PALETTE_RED), 0);
        }
        // 设置标题字体为ziti_title
        lv_obj_t * title_lbl = lv_msgbox_get_title(mbox);
        if(title_lbl) {
            lv_obj_set_style_text_font(title_lbl, &ziti_title, 0);
        }
    }

    // 7. 延时0.5秒后清空购物车（保留标题）
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
static void cart_action_btnmatrix_cb(lv_event_t * e);

// 购物车按钮项点击回调：点击商品项弹出操作菜单（删除/修改数量）
void cart_list_btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);
        if(btn == NULL) return;

        // 获取当前商品项的文字，提取商品名以定位 product
        const char * text = lv_list_get_btn_text(cart_list, btn);
        if(text == NULL) return;

        // 反向查找对应的商品（通过 user_data 传递给操作菜单回调）
        product_t * target_product = NULL;
        for(uint8_t j = 0; j < MAX_PRODUCTS; j++) {
            if(strncmp(text, shop_products[j].name, strlen(shop_products[j].name)) == 0) {
                target_product = &shop_products[j];
                break;
            }
        }

        // 创建操作选择弹窗（使用 btnmatrix），标题栏右侧带关闭按钮
        static const char * action_btns[] = {CN_DELETE_ITEM, CN_EDIT_QTY, ""};
        lv_obj_t * mbox = lv_msgbox_create(NULL, CN_CART_ACTION, NULL, action_btns, true);
        lv_obj_center(mbox);

        /* 获取消息框内部的按钮矩阵，并为其添加事件回调。
         * 回调通过 user_data 传入被操作的列表按钮（btn），
         * 根据选中的按钮索引执行删除或修改数量操作。
         */
        lv_obj_t * btnm = lv_msgbox_get_btns(mbox);
        if(btnm != NULL) {
            lv_obj_add_event_cb(btnm, cart_action_btnmatrix_cb, LV_EVENT_VALUE_CHANGED, btn);
        }
    }
}

/* 操作菜单 - 删除按钮回调 */
static void cart_item_delete_cb(lv_event_t * e)
{
    lv_obj_t * btn = (lv_obj_t *)lv_event_get_user_data(e);
    if(btn != NULL) {
        lv_obj_del(btn);
    }
    // 关闭弹窗（点击弹窗外部或按钮后 msgbox 自动关闭）
}

/* 操作菜单 - 修改数量按钮回调 */
static void cart_item_edit_qty_cb(lv_event_t * e)
{
    product_t * p = (product_t *)lv_event_get_user_data(e);
    if(p == NULL || input_ta == NULL || num_kb == NULL) return;

    current_product = p;

    // 设置输入模式为修改数量
    current_kb_mode = KB_MODE_EDIT_QUANTITY;

    // 动态设置文本框提示语
    static char prompt_str[96];
    snprintf(prompt_str, sizeof(prompt_str), "%s%s%s",
             CN_NEW_QTY_PREFIX, p->name, CN_NEW_QTY_MID);
    lv_textarea_set_placeholder_text(input_ta, prompt_str);

    // 显示输入界面
    show_input_ui();
}

/* 按钮矩阵事件回调：处理购物车操作（删除 / 修改数量） */
static void cart_action_btnmatrix_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    lv_obj_t * btnm = lv_event_get_target(e);
    int sel = (int)lv_btnmatrix_get_selected_btn(btnm);
    lv_obj_t * list_btn = (lv_obj_t *)lv_event_get_user_data(e);
    if(list_btn == NULL) return;

    if(sel == 0) {
        /* 删除商品 */
        lv_obj_del(list_btn);
    } else if(sel == 1) {
        /* 修改数量：根据按钮文本查找对应商品并进入编辑模式 */
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

    /* 关闭消息框 */
    lv_obj_t * mbox = lv_obj_get_parent(btnm);
    if(mbox) lv_msgbox_close(mbox);
}

// ==================== 交易历史面板 ====================

static lv_obj_t * hist_panel = NULL;    // 历史面板容器
static lv_obj_t * hist_list  = NULL;    // 交易列表（可滚动，显示全部记录）

// 前向声明
static void hist_list_item_cb(lv_event_t * e);
static void refresh_history_list(void);

/* 获取折扣描述文字 */
static const char * get_discount_desc_str(tx_discount_type_t type)
{
    switch(type) {
        case TX_DISC_FULL_REDUCTION: return CN_DESC_DISC_FULL20;
        case TX_DISC_PERCENT_OFF:    return CN_DESC_DISC_90PCT;
        case TX_DISC_FULL_100_80PCT: return CN_DESC_DISC_FULL100_80PCT;
        case TX_DISC_FULL_200_50:    return CN_DESC_DISC_FULL200_RED50;
        default:                     return CN_DESC_DISC_NONE;
    }
}

/* 刷新历史列表（显示全部记录，最新在前） */
static void refresh_history_list(void)
{
    if(hist_list == NULL) return;

    // 清空列表
    lv_obj_clean(hist_list);

    int total = (int)tx_log.count;

    // 无记录提示
    if(total == 0) {
        lv_obj_t * lbl = lv_label_create(hist_list);
        lv_label_set_text(lbl, CN_TX_EMPTY);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(lbl, lv_pct(100));
    } else {
        // 倒序显示（最新在前），全部记录
        for(int i = total - 1; i >= 0; i--) {
            transaction_t * tx = &tx_log.records[i];

            // 构建行文字: "#交易记录N | M项商品 | 实付: XX.XX 元"
            static char line_buf[128];
            if(tx->total_before_discount != tx->total_after_discount) {
                snprintf(line_buf, sizeof(line_buf),
                         "%s%lu | %d " CN_TX_ITEMS " | " CN_TX_PAY ": %.2f " CN_YUAN,
                         CN_TX_ID, (unsigned long)tx->id, (int)tx->item_count,
                         tx->total_after_discount);
            } else {
                snprintf(line_buf, sizeof(line_buf),
                         "%s%lu | %d " CN_TX_ITEMS " | " CN_TX_TOTAL ": %.2f " CN_YUAN,
                         CN_TX_ID, (unsigned long)tx->id, (int)tx->item_count,
                         tx->total_after_discount);
            }

            lv_obj_t * btn = lv_list_add_btn(hist_list, LV_SYMBOL_FILE, line_buf);
            lv_obj_add_event_cb(btn, hist_list_item_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        }
    }
}

/* 关闭历史面板（同时删除遮罩层） */
static void close_history_panel(void)
{
    if(hist_panel) {
        lv_obj_t * overlay = lv_obj_get_parent(hist_panel);
        lv_obj_del(overlay);          // 删除遮罩层，它会连带删除子对象 hist_panel
        hist_panel = NULL;
        hist_list = NULL;
    }
}

/* 关闭交易详情弹窗（删除遮罩层，连带删除其子对象 popup）*/
static void close_detail_popup(lv_event_t * e)
{
    lv_obj_t * overlay = (lv_obj_t *)lv_event_get_user_data(e);
    if(overlay) lv_obj_del(overlay);
}

/* 关闭面板回调 */
static void hist_close_cb(lv_event_t * e)
{
    (void)e;
    close_history_panel();
}

/* 点击交易行 → 显示详情弹窗 */
static void hist_list_item_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);

    if(idx < 0 || idx >= (int)tx_log.count) return;
    transaction_t * tx = &tx_log.records[idx];

    // 创建全屏半透明遮罩
    lv_obj_t * overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, 1024, 600);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(overlay, close_detail_popup, LV_EVENT_CLICKED, overlay);

    // 详情弹窗
    lv_obj_t * popup = lv_obj_create(overlay);
    lv_obj_set_size(popup, 500, 450);
    lv_obj_center(popup);
    lv_obj_set_style_bg_color(popup, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(popup, 2, 0);
    lv_obj_set_style_border_color(popup, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_radius(popup, 10, 0);
    lv_obj_set_flex_flow(popup, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(popup, 6, 0);
    lv_obj_set_style_pad_all(popup, 15, 0);
    lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(popup, LV_OBJ_FLAG_CLICKABLE); // 阻止点击穿透到遮罩

    // 标题
    lv_obj_t * title_lbl = lv_label_create(popup);
    static char title_buf[48];
    snprintf(title_buf, sizeof(title_buf), "%s #%lu", CN_TX_DETAIL, (unsigned long)tx->id);
    lv_label_set_text(title_lbl, title_buf);
    lv_obj_set_style_text_font(title_lbl, &ziti_title, 0);
    lv_obj_set_style_text_color(title_lbl, lv_palette_main(LV_PALETTE_BLUE), 0);

    // 金额信息
    lv_obj_t * money_lbl = lv_label_create(popup);
    static char money_buf[256];
    const char * disc_str = get_discount_desc_str(tx->discount_type);
    if(tx->total_before_discount != tx->total_after_discount) {
        snprintf(money_buf, sizeof(money_buf),
                 CN_BEFORE_DISC ": %.2f " CN_YUAN "\n"
                 CN_AFTER_DISC  ": %.2f " CN_YUAN "%s\n"
                 CN_TX_DISC_FMT,
                 tx->total_before_discount, tx->total_after_discount,
                 disc_str, disc_str);
    } else {
        snprintf(money_buf, sizeof(money_buf),
                 CN_TOTAL ": %.2f " CN_YUAN "\n" CN_DISC_NONE,
                 tx->total_after_discount);
    }
    lv_label_set_text(money_lbl, money_buf);
    lv_obj_set_style_text_color(money_lbl, lv_palette_main(LV_PALETTE_RED), 0);

    // 商品明细（可滚动，列布局）
    lv_obj_t * items_cont = lv_obj_create(popup);
    lv_obj_set_width(items_cont, lv_pct(100));
    lv_obj_set_flex_grow(items_cont, 1);
    lv_obj_set_flex_flow(items_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_border_width(items_cont, 0, 0);
    lv_obj_set_style_bg_opa(items_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_gap(items_cont, 4, 0);
    lv_obj_set_style_pad_all(items_cont, 0, 0);

    for(uint8_t j = 0; j < tx->item_count; j++) {
        uint8_t pid = tx->items[j].product_id;
        if(pid < MAX_PRODUCTS) {
            static char item_buf[96];
            snprintf(item_buf, sizeof(item_buf),
                     "%s " CN_QTY_X "%.1f %s = %.2f " CN_YUAN,
                     shop_products[pid].name,
                     tx->items[j].quantity,
                     shop_products[pid].unit,
                     tx->items[j].subtotal);

            lv_obj_t * item_lbl = lv_label_create(items_cont);
            lv_label_set_text(item_lbl, item_buf);
        }
    }

    // 确定/关闭按钮
    lv_obj_t * close_btn = lv_btn_create(popup);
    lv_obj_set_size(close_btn, lv_pct(60), 40);
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(close_btn, close_detail_popup, LV_EVENT_CLICKED, overlay);
    lv_obj_t * close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, CN_OK);
    lv_obj_center(close_lbl);
}

/* 清空交易记录回调 */
static void hist_clear_cb(lv_event_t * e)
{
    (void)e;
    tx_log_clear();
    refresh_history_list();
}

/* 显示交易历史面板（全屏遮罩） */
void show_history_panel(void)
{
    // 避免重复打开
    if(hist_panel) return;

    // 全屏遮罩
    lv_obj_t * overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, 1024, 600);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);

    hist_panel = lv_obj_create(overlay);
    lv_obj_set_size(hist_panel, 600, 520);
    lv_obj_center(hist_panel);
    lv_obj_set_style_bg_color(hist_panel, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(hist_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hist_panel, 2, 0);
    lv_obj_set_style_border_color(hist_panel, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_radius(hist_panel, 10, 0);
    lv_obj_set_flex_flow(hist_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(hist_panel, 8, 0);
    lv_obj_set_style_pad_all(hist_panel, 15, 0);
    lv_obj_clear_flag(hist_panel, LV_OBJ_FLAG_SCROLLABLE);

    // 主标题：交易明细（左对齐，醒目）
    lv_obj_t * main_title = lv_label_create(hist_panel);
    lv_label_set_text(main_title, CN_TX_DETAIL);
    lv_obj_set_style_text_font(main_title, &ziti_title, 0);
    lv_obj_set_style_text_color(main_title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_width(main_title, lv_pct(100));
    lv_obj_set_style_text_align(main_title, LV_TEXT_ALIGN_LEFT, 0);

    // 顶部：关闭按钮（右上角）
    lv_obj_t * top_row = lv_obj_create(hist_panel);
    lv_obj_set_size(top_row, lv_pct(100), 40);
    lv_obj_set_style_border_width(top_row, 0, 0);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(top_row, 0, 0);
    lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);

    // 关闭按钮（右上角）
    lv_obj_t * close_btn = lv_btn_create(top_row);
    lv_obj_set_size(close_btn, 80, 32);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(close_btn, hist_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, CN_CLOSE_BTN);
    lv_obj_center(close_lbl);

    // 副标题：最多储存 N 条记录
    static char max_buf[48];
    snprintf(max_buf, sizeof(max_buf), CN_MAX_RECORDS, MAX_TX_HISTORY);
    lv_obj_t * subtitle = lv_label_create(hist_panel);
    lv_label_set_text(subtitle, max_buf);
    lv_obj_set_style_text_color(subtitle, lv_palette_main(LV_PALETTE_GREY), 0);

    // 中部：交易列表（可滚动）
    hist_list = lv_list_create(hist_panel);
    lv_obj_set_width(hist_list, lv_pct(100));
    lv_obj_set_flex_grow(hist_list, 1);
    lv_obj_set_style_pad_gap(hist_list, 8, 0);

    // 底部：清空记录按钮
    lv_obj_t * bottom_row = lv_obj_create(hist_panel);
    lv_obj_set_size(bottom_row, lv_pct(100), 45);
    lv_obj_set_style_border_width(bottom_row, 0, 0);
    lv_obj_set_style_bg_opa(bottom_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(bottom_row, 0, 0);
    lv_obj_clear_flag(bottom_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * btn_clr = lv_btn_create(bottom_row);
    lv_obj_set_size(btn_clr, 120, 36);
    lv_obj_align(btn_clr, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(btn_clr, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_clr, hist_clear_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_clr = lv_label_create(btn_clr);
    lv_label_set_text(lbl_clr, CN_HISTORY_CLR);
    lv_obj_center(lbl_clr);

    // 首次加载数据
    refresh_history_list();
}

/* 交易记录按钮回调 */
void history_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        show_history_panel();
    }
}
