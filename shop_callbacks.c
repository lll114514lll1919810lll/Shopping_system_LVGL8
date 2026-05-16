#include "shop_app.h"
#include "shop_chinese.h"

// 优惠类型枚举
typedef enum {
    DISCOUNT_NONE = 0,
    DISCOUNT_FULL_REDUCTION,
    DISCOUNT_PERCENT_OFF
} discount_type_t;

// 获取当前选中的优惠（供结算使用）
discount_type_t get_current_discount(void);

static discount_type_t current_discount = DISCOUNT_NONE;

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

// 延时清空购物车的定时器回调
static void clear_cart_timer_cb(lv_timer_t * timer)
{
    if(cart_list != NULL) {
        uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);
        for(int i = child_cnt - 1; i > 0; i--) {
            lv_obj_del(lv_obj_get_child(cart_list, i));
        }
    }
    lv_timer_del(timer); // 一次性定时器，自行删除
}

void product_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        product_t * p = (product_t *)lv_event_get_user_data(e);
        if(p == NULL || input_ta == NULL || num_kb == NULL) return;

        current_product = p; // 存入静态变量

        // 动态设置文本框提示语（中文版）
        static char prompt_str[96];
        snprintf(prompt_str, sizeof(prompt_str), "%s%s%s%s%s", 
                 CN_QTY_PREFIX, p->name, CN_QTY_MID, p->unit, CN_QTY_SUFFIX);
        lv_textarea_set_placeholder_text(input_ta, prompt_str);
        
        // 显示输入界面
        lv_keyboard_set_textarea(num_kb, input_ta);
        lv_obj_clear_flag(input_ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(label_full);
        lv_obj_move_foreground(num_kb);
        lv_obj_move_foreground(input_ta);
    }
}

// --- B. 键盘确定键回调 (核心计算逻辑) ---
void kb_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    // 只有在按下键盘的“确定”（READY）键时才处理
    if(code == LV_EVENT_READY) {
        if(cart_list == NULL || input_ta == NULL || current_product == NULL) return;

        const char * input_str = lv_textarea_get_text(input_ta);
        if(input_str && strlen(input_str) > 0) {
            // 检查是否为按个卖的商品
            if(strcmp(current_product->unit, CN_BOX) == 0 ||
               strcmp(current_product->unit, CN_PACK) == 0 ||
               strcmp(current_product->unit, CN_BOTTLE) == 0) {
                // 检查输入是否为整数（不能有小数点）
                if(strchr(input_str, '.') != NULL) {
                    lv_obj_t * mbox = lv_msgbox_create(NULL, CN_ERROR, CN_INT_ONLY, NULL, true);
                    lv_obj_center(mbox);
                    // 隐藏输入界面
                    lv_obj_add_flag(input_ta, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
                    lv_textarea_set_text(input_ta, "");
                    lv_obj_move_background(label_full);
                    return;
                }
            }
            float new_weight = (float)atof(input_str);
            if(new_weight < 0) return; // 忽略无效输入

            float total_price = new_weight * (float)current_product->price;
            
            // 准备新的文字内容
            static char buf[128];
            snprintf(buf, sizeof(buf), "%s x%.1f %s = %.2f " CN_YUAN, 
                     current_product->name, new_weight, current_product->unit, total_price);

            // --- 核心逻辑：寻找购物车中是否已存在该商品 ---
            lv_obj_t * existing_btn = NULL;
            uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);
            
            for(uint32_t i = 0; i < child_cnt; i++) {
                lv_obj_t * child = lv_obj_get_child(cart_list, i);
                const char * text = lv_list_get_btn_text(cart_list, child);
                
                // 检查列表里的文字是否是以当前商品名开头的
                if(text && strncmp(text, current_product->name, strlen(current_product->name)) == 0) {
                    existing_btn = child;
                    break;
                }
            }

            if(existing_btn) {
                // 删除旧项，再新增（避免直接修改 label 子对象导致文字追加）
                lv_obj_del(existing_btn);
            }
            // 新增一行按钮，并注册点击删除事件
            lv_obj_t* btn = lv_list_add_btn(cart_list, LV_SYMBOL_OK, buf);
            lv_obj_add_event_cb(btn, cart_list_btn_event_cb, LV_EVENT_CLICKED, NULL);
        }

        // 无论是否添加成功，都重置并隐藏输入界面
        lv_obj_add_flag(input_ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(input_ta, "");
		lv_obj_move_background(label_full);
    }
    else if(code == LV_EVENT_CANCEL) {
        lv_obj_add_flag(input_ta, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_background(label_full);
    }
}

// --- C. 结算按钮回调 (解析字符串求和) ---
void checkout_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(cart_list == NULL) return;

    float grand_total = 0.0f;
    uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);
    
    // 1. 检查购物车是否为空
    if(child_cnt == 0) {
        lv_obj_t * mbox = lv_msgbox_create(NULL, CN_HINT, CN_CART_EMPTY, NULL, true);
        lv_obj_center(mbox);
        return;
    }

    // 2. 遍历列表，解析 "=" 后的价格并累加
    for(uint32_t i = 0; i < child_cnt; i++) {
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
                discount_desc = CN_DESC_FULL;
            }
            break;
        case DISCOUNT_PERCENT_OFF:      // 9折
            final_total = grand_total * 0.9f;
            discount_desc = CN_DESC_90PCT;
            break;
        case DISCOUNT_NONE:
        default:
            discount_desc = CN_DESC_NONE;
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

		// 5. 准备结果字符串 (扩大缓冲区，容纳折扣信息)
    static char result_buf[128];
    if (grand_total != final_total) {
        snprintf(result_buf, sizeof(result_buf),
                 CN_BEFORE_DISC ": %.2f " CN_YUAN "\n" CN_AFTER_DISC ": %.2f " CN_YUAN "%s",
                 grand_total, final_total, discount_desc);
    } else {
        snprintf(result_buf, sizeof(result_buf),
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

// 购物车按钮项点击回调：点击商品项则删除该项
void cart_list_btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t* btn = lv_event_get_target(e);
        lv_obj_del(btn);
    }
}

// ==================== 交易历史面板 ====================
#define HIST_PAGE_SIZE  5   // 每页显示条数

static lv_obj_t * hist_panel = NULL;    // 历史面板容器
static lv_obj_t * hist_list  = NULL;    // 交易列表
static lv_obj_t * hist_page_label = NULL; // 页码标签
static int hist_current_page = 0;       // 当前页（0-based）
static int hist_total_pages  = 1;       // 总页数

// 前向声明
static void hist_list_item_cb(lv_event_t * e);
static void refresh_history_list(void);

/* 获取折扣描述文字 */
static const char * get_discount_desc_str(tx_discount_type_t type)
{
    switch(type) {
        case TX_DISC_FULL_REDUCTION: return CN_DESC_FULL;
        case TX_DISC_PERCENT_OFF:    return CN_DESC_90PCT;
        default:                     return CN_DESC_NONE;
    }
}

/* 刷新历史列表（根据当前页码重绘） */
static void refresh_history_list(void)
{
    if(hist_list == NULL) return;

    // 清空列表
    lv_obj_clean(hist_list);

    int total = (int)tx_log.count;
    hist_total_pages = (total + HIST_PAGE_SIZE - 1) / HIST_PAGE_SIZE;
    if(hist_total_pages < 1) hist_total_pages = 1;

    // 边界修正
    if(hist_current_page >= hist_total_pages) hist_current_page = hist_total_pages - 1;
    if(hist_current_page < 0) hist_current_page = 0;

    int start_idx = hist_current_page * HIST_PAGE_SIZE;
    int end_idx = start_idx + HIST_PAGE_SIZE;
    if(end_idx > total) end_idx = total;

    // 无记录提示
    if(total == 0) {
        lv_obj_t * lbl = lv_label_create(hist_list);
        lv_label_set_text(lbl, CN_TX_EMPTY);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(lbl, lv_pct(100));
    } else {
        // 倒序显示（最新在前）
        for(int i = end_idx - 1; i >= start_idx; i--) {
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

    // 更新页码文字
    if(hist_page_label) {
        static char page_buf[32];
        snprintf(page_buf, sizeof(page_buf), CN_PAGE_FMT,
                 hist_current_page + 1, hist_total_pages);
        lv_label_set_text(hist_page_label, page_buf);
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
        hist_page_label = NULL;
        hist_current_page = 0;
    }
}

/* 关闭交易详情弹窗 */
static void close_detail_popup(lv_event_t * e)
{
    lv_obj_t * popup = (lv_obj_t *)lv_event_get_user_data(e);
    if(popup) lv_obj_del(popup);
}

/* 翻页回调 */
static void hist_prev_cb(lv_event_t * e)
{
    (void)e;
    if(hist_current_page > 0) {
        hist_current_page--;
        refresh_history_list();
    }
}

static void hist_next_cb(lv_event_t * e)
{
    (void)e;
    if(hist_current_page < hist_total_pages - 1) {
        hist_current_page++;
        refresh_history_list();
    }
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
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title_lbl, lv_palette_main(LV_PALETTE_BLUE), 0);

    // 金额信息
    lv_obj_t * money_lbl = lv_label_create(popup);
    static char money_buf[160];
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
                 CN_TOTAL ": %.2f " CN_YUAN "\n" CN_NO_DISC,
                 tx->total_after_discount);
    }
    lv_label_set_text(money_lbl, money_buf);
    lv_obj_set_style_text_color(money_lbl, lv_palette_main(LV_PALETTE_RED), 0);

    // 商品明细（可滚动）
    lv_obj_t * items_cont = lv_obj_create(popup);
    lv_obj_set_width(items_cont, lv_pct(100));
    lv_obj_set_flex_grow(items_cont, 1);
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
    hist_current_page = 0;
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

    // 顶部：标题 + 关闭按钮
    lv_obj_t * top_row = lv_obj_create(hist_panel);
    lv_obj_set_size(top_row, lv_pct(100), 40);
    lv_obj_set_style_border_width(top_row, 0, 0);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(top_row, 0, 0);
    lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title = lv_label_create(top_row);
    lv_label_set_text(title, CN_HISTORY_TITLE);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t * close_btn = lv_btn_create(top_row);
    lv_obj_set_size(close_btn, 80, 32);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(close_btn, hist_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, CN_CLOSE_BTN);
    lv_obj_center(close_lbl);

    // 中部：交易列表（可滚动）
    hist_list = lv_list_create(hist_panel);
    lv_obj_set_width(hist_list, lv_pct(100));
    lv_obj_set_flex_grow(hist_list, 1);
    lv_obj_set_style_pad_gap(hist_list, 8, 0);

    // 底部：翻页 + 清空
    lv_obj_t * bottom_row = lv_obj_create(hist_panel);
    lv_obj_set_size(bottom_row, lv_pct(100), 45);
    lv_obj_set_style_border_width(bottom_row, 0, 0);
    lv_obj_set_style_bg_opa(bottom_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(bottom_row, 0, 0);
    lv_obj_clear_flag(bottom_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * btn_prev = lv_btn_create(bottom_row);
    lv_obj_set_size(btn_prev, 80, 36);
    lv_obj_align(btn_prev, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_prev, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_prev, hist_prev_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_prev = lv_label_create(btn_prev);
    lv_label_set_text(lbl_prev, CN_PAGE_PREV);
    lv_obj_center(lbl_prev);

    hist_page_label = lv_label_create(bottom_row);
    lv_obj_align(hist_page_label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(hist_page_label, "");

    lv_obj_t * btn_next = lv_btn_create(bottom_row);
    lv_obj_set_size(btn_next, 80, 36);
    lv_obj_align(btn_next, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_next, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_next, hist_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_next = lv_label_create(btn_next);
    lv_label_set_text(lbl_next, CN_PAGE_NEXT);
    lv_obj_center(lbl_next);

    // 清空记录按钮
    lv_obj_t * btn_clr = lv_btn_create(bottom_row);
    lv_obj_set_size(btn_clr, 90, 36);
    lv_obj_align(btn_clr, LV_ALIGN_RIGHT_MID, -95, 0);
    lv_obj_set_style_bg_color(btn_clr, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_clr, hist_clear_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_clr = lv_label_create(btn_clr);
    lv_label_set_text(lbl_clr, CN_HISTORY_CLR);
    lv_obj_center(lbl_clr);

    // 首次加载数据
    hist_current_page = 0;
    refresh_history_list();
}

/* 交易记录按钮回调 */
void history_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        show_history_panel();
    }
}
