#include "shop_app.h"
#include "shop_chinese.h"

// SDRAM 模拟堆管理 (从 4MB 偏移处开始)
static uint32_t sdram_heap_ptr = 0xC0000000 + (1024 * 1024 * 4); 

void * sdram_malloc(uint32_t size) {
    void * p = (void *)sdram_heap_ptr;
    sdram_heap_ptr += size; 
    if(sdram_heap_ptr % 4 != 0) sdram_heap_ptr += (4 - (sdram_heap_ptr % 4));
    return p;
}

// 商品数据定义
product_t shop_products[MAX_PRODUCTS] = {
    {0, CN_APPLE,      8,  "kg",        "0:/apple.bin"},
    {1, CN_MILK,       6,  CN_BOX,      "0:/milk.bin"},
    {2, CN_BREAD,      10, CN_PACK,     "0:/bread.bin"},
    {3, CN_WATERMELON, 3,  "kg",        "0:/watermelon.bin"},
    {4, CN_COLA,       3,  CN_BOTTLE,   "0:/cola.bin"},
    {5, CN_CHOCOLATE,  40, CN_BOX,      "0:/chocolate.bin"}
};

lv_obj_t * cart_list = NULL;
lv_obj_t * input_ta = NULL;
lv_obj_t * num_kb = NULL;
lv_obj_t * label_full = NULL;
static lv_img_dsc_t img_dscs[MAX_PRODUCTS]; // 存储图片描述符

void shop_ui_init(void)
{
    lv_obj_t * scr = lv_scr_act();
    uint32_t img_size_with_header = 200 * 200 * 2 + 4; 
    uint32_t pure_data_size = 200 * 200 * 2;

    // 1. 左侧容器
    lv_obj_t * product_panel = lv_obj_create(scr);
    lv_obj_set_size(product_panel, 520, 580);
    lv_obj_set_pos(product_panel, 10, 10);
    lv_obj_set_flex_flow(product_panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(product_panel, 15, 0);

    // 2. 循环加载图片并生成卡片
    for(int i = 0; i < MAX_PRODUCTS; i++) {
        // 分配并读取 SD 卡图片到 SDRAM
        uint8_t * image_buffer = (uint8_t *)sdram_malloc(img_size_with_header);
        read_file_to_array(shop_products[i].img_path, image_buffer, img_size_with_header);

        // 初始化描述符
        img_dscs[i].header.always_zero = 0;
        img_dscs[i].header.cf = LV_IMG_CF_TRUE_COLOR;
        img_dscs[i].header.w = 200;
        img_dscs[i].header.h = 200;
        img_dscs[i].data_size = pure_data_size;
        img_dscs[i].data = image_buffer + 4;

        // 创建卡片容器
        lv_obj_t * item_cont = lv_obj_create(product_panel);
        lv_obj_set_size(item_cont, 220, 280); 
        lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(item_cont, 2, 0);
        lv_obj_set_style_pad_top(item_cont, 0, 0);

        // 创建图片
        lv_obj_t * img = lv_img_create(item_cont);
        lv_img_set_src(img, &img_dscs[i]);
        lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);

        // 创建商品名标签（大字体）
        lv_obj_t * name_label = lv_label_create(item_cont);
        lv_label_set_text(name_label, shop_products[i].name);
        lv_obj_set_style_text_font(name_label, &ziti_title, 0);
        lv_obj_align(name_label, LV_ALIGN_TOP_MID, 0, 208);
        lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_CENTER, 0);

        // 创建价格标签（小字体）
        lv_obj_t * price_label = lv_label_create(item_cont);
        lv_label_set_text_fmt(price_label, "#ff0000 %d " CN_YUAN "# / %s",
                              shop_products[i].price,
                              shop_products[i].unit);
        lv_label_set_recolor(price_label, true);
        lv_obj_set_style_text_font(price_label, &ziti, 0);
        lv_obj_align(price_label, LV_ALIGN_TOP_MID, 0, 245);
        lv_obj_set_style_text_align(price_label, LV_TEXT_ALIGN_CENTER, 0);

        // 绑定事件
        lv_obj_add_flag(item_cont, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(item_cont, product_btn_event_cb, LV_EVENT_CLICKED, &shop_products[i]);
    }

    // 3. 右侧购物车
    cart_list = lv_list_create(scr);
    lv_obj_set_size(cart_list, 240, 450);
    lv_obj_set_pos(cart_list, 540, 10);
    lv_obj_set_style_pad_gap(cart_list, 12, 0);
    lv_obj_set_style_pad_all(cart_list, 10, 0);

    // 标题
    lv_obj_t * cart_title = lv_label_create(cart_list);
    lv_label_set_text(cart_title, CN_CART_TITLE);
    lv_obj_set_style_text_font(cart_title, &ziti_title, 0);
    lv_obj_set_style_text_align(cart_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(cart_title, lv_pct(100));

    // 4. 输入框与键盘
    input_ta = lv_textarea_create(scr);
    lv_obj_set_size(input_ta, 300, 50);
    lv_obj_align(input_ta, LV_ALIGN_CENTER, 0, -100);
    lv_textarea_set_one_line(input_ta, true);
    lv_obj_add_flag(input_ta, LV_OBJ_FLAG_HIDDEN);

    num_kb = lv_keyboard_create(scr);
    lv_keyboard_set_mode(num_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(num_kb, kb_event_cb, LV_EVENT_ALL, NULL); 

    label_full = lv_label_create(scr);
    lv_label_set_text(label_full, "");
    lv_obj_set_size(label_full, 1024, 600);
    lv_obj_move_background(label_full);
    lv_obj_add_flag(label_full, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(label_full, LV_OBJ_FLAG_HIDDEN);  // 默认隐藏
    lv_obj_add_event_cb(label_full, label_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(label_full, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(label_full, LV_OPA_60, 0);

    // 5. 底部控制按钮
    lv_obj_t * btn_checkout = lv_btn_create(scr);
    lv_obj_set_size(btn_checkout, 110, 50);
    lv_obj_set_pos(btn_checkout, 540, 530);
    lv_obj_set_style_bg_color(btn_checkout, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_checkout, checkout_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_checkout = lv_label_create(btn_checkout);
    lv_label_set_text(lbl_checkout, CN_CHECKOUT);
    lv_obj_center(lbl_checkout);

    lv_obj_t * btn_clear = lv_btn_create(scr);
    lv_obj_set_size(btn_clear, 110, 50);
    lv_obj_set_pos(btn_clear, 670, 530); 
    lv_obj_set_style_bg_color(btn_clear, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_clear, clear_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_clear = lv_label_create(btn_clear);
    lv_label_set_text(lbl_clear, CN_CLEAR);
    lv_obj_center(lbl_clear);

    // 5b. 交易记录按钮
    lv_obj_t * btn_history = lv_btn_create(scr);
    lv_obj_set_size(btn_history, 110, 50);
    lv_obj_set_pos(btn_history, 900, 530);
    lv_obj_set_style_bg_color(btn_history, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(btn_history, history_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_history = lv_label_create(btn_history);
    lv_label_set_text(lbl_history, CN_HISTORY);
    lv_obj_center(lbl_history);

    //6.优惠
		lv_obj_t * discount_panel = lv_obj_create(scr);
		lv_obj_set_size(discount_panel, 220, 450);
		lv_obj_set_pos(discount_panel, 790, 10);
		lv_obj_set_flex_flow(discount_panel, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_style_pad_gap(discount_panel, 12, 0);
		lv_obj_set_style_pad_all(discount_panel, 10, 0);

			// 标题
		lv_obj_t * discount_title = lv_label_create(discount_panel);
		lv_label_set_text(discount_title, CN_DISCOUNTS);
		lv_obj_set_style_text_font(discount_title, &ziti_title, 0);
		lv_obj_set_style_text_align(discount_title, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_width(discount_title, lv_pct(100));

		// 复选框选项（互斥组）
		static const char * discount_opts[] = {CN_DISC_NONE, CN_DISC_FULL20_RED5, CN_DISC_90PCT, CN_DISC_FULL100_80PCT, CN_DISC_FULL200_RED50, NULL};
		for (int i = 0; discount_opts[i] != NULL; i++) {
				lv_obj_t * cb = lv_checkbox_create(discount_panel);
				lv_checkbox_set_text(cb, discount_opts[i]);
				// 绑定回调，并传递选项索引
				lv_obj_add_event_cb(cb, discount_cb_event_cb, LV_EVENT_VALUE_CHANGED, (void *)(uintptr_t)i);
				// 默认选中“无优惠”
				if (i == 0) {
						lv_obj_add_state(cb, LV_STATE_CHECKED);
				}
		}
}

// ==================== UI 辅助函数 ====================

/**
 * @brief 显示通用弹窗
 * @param title 弹窗标题
 * @param message 弹窗内容
 * @param txt_color 文本颜色（传 NULL 使用默认颜色）
 * @return 弹窗对象指针
 */
lv_obj_t * shop_ui_show_msgbox(const char * title, const char * message, const lv_color_t * txt_color)
{
    lv_obj_t * mbox = lv_msgbox_create(NULL, title, message, NULL, true);
    lv_obj_center(mbox);
    
    // 设置文本颜色（传 NULL 则不修改颜色）
    if(txt_color != NULL) {
        lv_obj_t * mbox_txt = lv_msgbox_get_text(mbox);
        if(mbox_txt) {
            lv_obj_set_style_text_color(mbox_txt, *txt_color, 0);
        }
    }
    
    // 设置标题字体
    lv_obj_t * title_lbl = lv_msgbox_get_title(mbox);
    if(title_lbl) {
        lv_obj_set_style_text_font(title_lbl, &ziti_title, 0);
    }
    
    return mbox;
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

/**
 * @brief 添加或更新购物车项（已存在则替换）
 * @param item_text 购物车项显示文本
 * @param product_name 商品名（用于查找去重）
 */
void shop_ui_add_cart_item(const char * item_text, const char * product_name)
{
    if(cart_list == NULL || item_text == NULL || product_name == NULL) return;
    
    // 查找是否已存在，若存在则删除旧项
    lv_obj_t * existing_btn = find_cart_item_by_name(product_name);
    if(existing_btn != NULL) {
        lv_obj_del(existing_btn);
    }
    
    // 添加新项
    lv_obj_t * btn = lv_list_add_btn(cart_list, LV_SYMBOL_OK, item_text);
    lv_obj_add_event_cb(btn, cart_list_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

/**
 * @brief 显示结算结果弹窗
 * @param tx          已构建的交易记录
 * @param grand_total 折扣前总价
 * @param final_total 折扣后总价
 * @param discount_desc 折扣描述文字
 */
void shop_ui_show_checkout_result(const transaction_t * tx, float grand_total, float final_total, const char * discount_desc)
{
    static char result_buf[512];
    int offset = 0;

    // 列出每个商品的明细
    for(uint8_t k = 0; k < tx->item_count; k++) {
        uint8_t pid = tx->items[k].product_id;
        if(pid < MAX_PRODUCTS) {
            offset += snprintf(result_buf + offset, sizeof(result_buf) - offset,
                               "%s x%.1f %s = %.2f " CN_YUAN "\n",
                               shop_products[pid].name,
                               tx->items[k].quantity,
                               shop_products[pid].unit,
                               tx->items[k].subtotal);
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

    // 创建弹窗
    static lv_color_t red_color;
    red_color = lv_palette_main(LV_PALETTE_RED);
    shop_ui_show_msgbox(CN_SUCCESS, result_buf, &red_color);
}

/**
 * @brief 显示购物车操作选择弹窗
 * @return 弹窗对象指针（用于绑定事件回调）
 */
lv_obj_t * shop_ui_show_cart_action_menu(void)
{
    static const char * action_btns[] = {CN_DELETE_ITEM, CN_EDIT_QTY, ""};
    lv_obj_t * mbox = lv_msgbox_create(NULL, CN_CART_ACTION, NULL, action_btns, true);
    lv_obj_center(mbox);
    return mbox;
}

// ==================== 交易历史面板辅助 ====================

static lv_obj_t * hist_panel = NULL;    // 历史面板容器
static lv_obj_t * hist_list  = NULL;    // 交易列表（可滚动，显示全部记录）

static void hist_list_item_cb(lv_event_t * e);  // 前向声明
static void hist_close_cb(lv_event_t * e) { (void)e; shop_ui_close_history_panel(); }
static void hist_clear_cb(lv_event_t * e) { (void)e; tx_log_clear(); shop_ui_refresh_history_list(); }

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

/* 关闭交易详情弹窗（删除遮罩层，连带删除其子对象 popup）*/
static void close_detail_popup(lv_event_t * e)
{
    lv_obj_t * overlay = (lv_obj_t *)lv_event_get_user_data(e);
    if(overlay) lv_obj_del(overlay);
}

/**
 * @brief 显示交易详情弹窗
 * @param tx 交易记录指针
 */
void shop_ui_show_tx_detail(transaction_t * tx)
{
    if(tx == NULL) return;

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

/* 点击交易行 → 显示详情弹窗（回调） */
static void hist_list_item_cb(lv_event_t * e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if(idx < 0 || idx >= (int)tx_log.count) return;
    shop_ui_show_tx_detail(&tx_log.records[idx]);
}

/* 刷新历史列表（显示全部记录，最新在前） */
void shop_ui_refresh_history_list(void)
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
void shop_ui_close_history_panel(void)
{
    if(hist_panel) {
        lv_obj_t * overlay = lv_obj_get_parent(hist_panel);
        lv_obj_del(overlay);          // 删除遮罩层，它会连带删除子对象 hist_panel
        hist_panel = NULL;
        hist_list = NULL;
    }
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
    shop_ui_refresh_history_list();
}
