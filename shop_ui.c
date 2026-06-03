#include "shop_app.h"
#include "shop_chinese.h"


// 优惠券复选框引用及基文本（供更新显示使用）
lv_obj_t * discount_checkboxes[5] = {NULL};
static const char * discount_base_texts[] = {
    CN_DISC_NONE, CN_DISC_FULL20_RED5, CN_DISC_90PCT, CN_DISC_FULL100_80PCT, CN_DISC_FULL200_RED50
};

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

// 页面管理
static lv_obj_t * home_screen = NULL;        // 主页屏幕引用
static lv_obj_t * shop_screen = NULL;        // 购物界面屏幕引用
static lv_obj_t * history_screen = NULL;     // 交易历史页面
static lv_obj_t * coupon_mgmt_screen = NULL; // 优惠券管理页面
static lv_obj_t * coupon_count_labels[4] = {NULL}; // 管理页面中4种优惠券的数量标签
static lv_obj_t * cart_total_label = NULL;   // 购物车实时总价标签

// 主页背景图片
static lv_img_dsc_t bg_img_dsc;
static uint8_t * bg_img_buffer = NULL;

// 前向声明
static void shop_btn_cb(lv_event_t * e);
static void history_btn_home_cb(lv_event_t * e);
static void back_btn_cb(lv_event_t * e);
static void coupon_mgmt_btn_home_cb(lv_event_t * e);
static void coupon_mgmt_plus_minus_cb(lv_event_t * e);
static void coupon_mgmt_reset_cb(lv_event_t * e);
static void shop_ui_update_coupon_mgmt_display(void);

/* 创建主页 */
void show_home_screen(void)
{
    // 避免重复创建
    if(home_screen) {
        lv_scr_load_anim(home_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        return;
    }

    // 创建新的屏幕
    home_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(home_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(home_screen, LV_OPA_COVER, 0);

    // 加载主页背景图片 (1024x600)
    if (bg_img_buffer == NULL) {
        uint32_t bg_data_size = 1024 * 600 * 2;   // RGB565
        uint32_t bg_file_size = bg_data_size + 4;  // +4 byte LVGL header
        bg_img_buffer = (uint8_t *)sdram_malloc(bg_file_size);
        read_file_to_array("0:/background.bin", bg_img_buffer, bg_file_size);
        bg_img_dsc.header.always_zero = 0;
        bg_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
        bg_img_dsc.header.w = 1024;
        bg_img_dsc.header.h = 600;
        bg_img_dsc.data_size = bg_data_size;
        bg_img_dsc.data = bg_img_buffer + 4;
    }
    lv_obj_t * bg_img = lv_img_create(home_screen);
    lv_img_set_src(bg_img, &bg_img_dsc);
    lv_obj_set_size(bg_img, 1024, 600);
    lv_obj_set_pos(bg_img, 0, 0);
    lv_obj_move_background(bg_img);  // 置于最底层

    // 购物按钮
    lv_obj_t * btn_shop = lv_btn_create(home_screen);
    lv_obj_set_size(btn_shop, 200, 80);
    lv_obj_align(btn_shop, LV_ALIGN_CENTER, -120, 50);
    lv_obj_set_style_bg_color(btn_shop, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_shop, shop_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_shop = lv_label_create(btn_shop);
    lv_label_set_text(lbl_shop, CN_BTN_SHOP);
    lv_obj_set_style_text_font(lbl_shop, &ziti_max, 0);
    lv_obj_center(lbl_shop);

    // 交易记录按钮
    lv_obj_t * btn_history = lv_btn_create(home_screen);
    lv_obj_set_size(btn_history, 200, 80);
    lv_obj_align(btn_history, LV_ALIGN_CENTER, 120, 50);
    lv_obj_set_style_bg_color(btn_history, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(btn_history, history_btn_home_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_history = lv_label_create(btn_history);
    lv_label_set_text(lbl_history, CN_BTN_HISTORY);
    lv_obj_set_style_text_font(lbl_history, &ziti_max, 0);
    lv_obj_center(lbl_history);

    // coupon management button
    lv_obj_t * btn_manage = lv_btn_create(home_screen);
    lv_obj_set_size(btn_manage, 200, 80);
    lv_obj_align(btn_manage, LV_ALIGN_CENTER, 0, 160);
    lv_obj_set_style_bg_color(btn_manage, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_add_event_cb(btn_manage, coupon_mgmt_btn_home_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_manage = lv_label_create(btn_manage);
    lv_label_set_text(lbl_manage, CN_COUPON_MGMT);
    lv_obj_set_style_text_font(lbl_manage, &ziti_max, 0);
    lv_obj_center(lbl_manage);

    // 切换到主页
    lv_scr_load_anim(home_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
}

/* 主页购物按钮回调 */
static void shop_btn_cb(lv_event_t * e)
{
    (void)e;
    show_shop_screen();
}

/* 主页交易记录按钮回调 */
static void history_btn_home_cb(lv_event_t * e)
{
    (void)e;
    show_history_panel();
}

/* 返回主页按钮回调（购物界面和交易记录界面共用） */
static void back_btn_cb(lv_event_t * e)
{
    (void)e;
    // 只切换屏幕，不删除任何屏幕，防止操作当前活动屏幕导致崩溃
    show_home_screen();
}

/* 显示购物界面 */
void show_shop_screen(void)
{
    // 避免重复创建
    if(shop_screen) {
        lv_scr_load_anim(shop_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        shop_ui_update_coupon_display();
        return;
    }

    // 创建购物界面屏幕
    shop_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(shop_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(shop_screen, LV_OPA_COVER, 0);

    uint32_t img_size_with_header = 200 * 200 * 2 + 4; 
    uint32_t pure_data_size = 200 * 200 * 2;

    // 1. 左侧容器
    lv_obj_t * product_panel = lv_obj_create(shop_screen);
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
    cart_list = lv_list_create(shop_screen);
    lv_obj_set_size(cart_list, 240, 425);
    lv_obj_set_pos(cart_list, 540, 10);
    lv_obj_set_style_pad_gap(cart_list, 12, 0);
    lv_obj_set_style_pad_all(cart_list, 10, 0);

    // 标题
    lv_obj_t * cart_title = lv_label_create(cart_list);
    lv_label_set_text(cart_title, CN_CART_TITLE);
    lv_obj_set_style_text_font(cart_title, &ziti_title, 0);
    lv_obj_set_style_text_align(cart_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(cart_title, lv_pct(100));

    // 购物车底部实时总价标签
    cart_total_label = lv_label_create(shop_screen);
    lv_obj_set_pos(cart_total_label, 550, 440);
    lv_obj_set_width(cart_total_label, 220);
    lv_obj_set_style_text_font(cart_total_label, &ziti_title, 0);
    lv_obj_set_style_text_align(cart_total_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(cart_total_label, CN_CART_TOTAL "0.00 " CN_YUAN);

    // 4. 输入框与键盘
    input_ta = lv_textarea_create(shop_screen);
    lv_obj_set_size(input_ta, 300, 50);
    lv_obj_align(input_ta, LV_ALIGN_CENTER, 0, -100);
    lv_textarea_set_one_line(input_ta, true);
    lv_obj_add_flag(input_ta, LV_OBJ_FLAG_HIDDEN);

    num_kb = lv_keyboard_create(shop_screen);
    lv_keyboard_set_mode(num_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(num_kb, kb_event_cb, LV_EVENT_ALL, NULL); 

    label_full = lv_label_create(shop_screen);
    lv_label_set_text(label_full, "");
    lv_obj_set_size(label_full, 1024, 600);
    lv_obj_move_background(label_full);
    lv_obj_add_flag(label_full, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(label_full, LV_OBJ_FLAG_HIDDEN);  // 默认隐藏
    lv_obj_add_event_cb(label_full, label_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(label_full, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(label_full, LV_OPA_60, 0);

    // 5. 底部控制按钮
    lv_obj_t * btn_checkout = lv_btn_create(shop_screen);
    lv_obj_set_size(btn_checkout, 110, 50);
    lv_obj_set_pos(btn_checkout, 540, 530);
    lv_obj_set_style_bg_color(btn_checkout, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_checkout, checkout_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_checkout = lv_label_create(btn_checkout);
    lv_label_set_text(lbl_checkout, CN_CHECKOUT);
    lv_obj_center(lbl_checkout);

    lv_obj_t * btn_clear = lv_btn_create(shop_screen);
    lv_obj_set_size(btn_clear, 110, 50);
    lv_obj_set_pos(btn_clear, 670, 530); 
    lv_obj_set_style_bg_color(btn_clear, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_clear, clear_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_clear = lv_label_create(btn_clear);
    lv_label_set_text(lbl_clear, CN_CLEAR);
    lv_obj_center(lbl_clear);

    // 5b. 返回主页按钮（替代原来的交易记录按钮）
    lv_obj_t * btn_back = lv_btn_create(shop_screen);
    lv_obj_set_size(btn_back, 110, 50);
    lv_obj_set_pos(btn_back, 900, 530);
    lv_obj_set_style_bg_color(btn_back, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(btn_back, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, CN_BTN_BACK);
    lv_obj_center(lbl_back);

    // 6. 优惠面板
    lv_obj_t * discount_panel = lv_obj_create(shop_screen);
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

    // 复选框选项（互斥组，带优惠券数量）
    for (int i = 0; i < 5; i++) {
        lv_obj_t * cb = lv_checkbox_create(discount_panel);
        discount_checkboxes[i] = cb;
        // 第0项"无优惠"不显示数量，其余显示剩余张数
        if (i == 0) {
            lv_checkbox_set_text(cb, discount_base_texts[i]);
        } else {
            char buf[64];
            snprintf(buf, sizeof(buf), CN_COUPON_REMAINING, discount_base_texts[i], coupon_remaining[i]);
            lv_checkbox_set_text(cb, buf);
            // 根据剩余数量设置文字颜色（>0绿色，=0红色）
            lv_obj_set_style_text_color(cb,
                coupon_remaining[i] > 0 ? lv_color_hex(0x00C800) : lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
            if (coupon_remaining[i] <= 0) {
                lv_obj_add_state(cb, LV_STATE_DISABLED);
            }
        }
        // 绑定回调，并传递选项索引
        lv_obj_add_event_cb(cb, discount_cb_event_cb, LV_EVENT_VALUE_CHANGED, (void *)(uintptr_t)i);
    }
    // 恢复上次选中的优惠（若已用完则回退到"无优惠"）
    if (current_discount > 0 && coupon_remaining[current_discount] > 0) {
        lv_obj_clear_state(discount_checkboxes[0], LV_STATE_CHECKED);
        lv_obj_add_state(discount_checkboxes[current_discount], LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(discount_checkboxes[0], LV_STATE_CHECKED);
        current_discount = 0;
    }

    // 初始应用门槛检查和颜色状态
    shop_ui_update_coupon_display();

    // 切换到购物界面
    lv_scr_load_anim(shop_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

/* 关闭购物界面 */
void shop_ui_close_shop_screen(void)
{
    if(shop_screen) {
        shop_screen = NULL;
        cart_list = NULL;
        input_ta = NULL;
        num_kb = NULL;
        label_full = NULL;
        cart_total_label = NULL;
    }
}

/* 计算购物车当前总价（跳过标题行，解析 "=" 后价格累加） */
static float get_cart_total(void)
{
    if(cart_list == NULL) return 0.0f;

    float total = 0.0f;
    uint32_t child_cnt = lv_obj_get_child_cnt(cart_list);

    for(uint32_t i = 1; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cart_list, i);
        const char * text = lv_list_get_btn_text(cart_list, child);
        if(text) {
            const char * eq = strchr(text, '=');
            if(eq) {
                total += (float)atof(eq + 1);
            }
        }
    }
    return total;
}

/* 更新购物车底部实时总价显示 */
void shop_ui_update_cart_total(void)
{
    if(cart_total_label == NULL) return;

    float total = get_cart_total();

    // 根据总价设置颜色：>0红色高亮，=0灰色
    // 手动拆分整数/小数，避免嵌入式 printf 不支持 %f
    static char total_buf[64];
    if(total > 0.0f) {
        lv_obj_set_style_text_color(cart_total_label, lv_palette_main(LV_PALETTE_RED), 0);
        int yuan = (int)total;
        int fen  = (int)((total - (float)yuan) * 100.0f + 0.5f);
        snprintf(total_buf, sizeof(total_buf), CN_CART_TOTAL "%d.%02d " CN_YUAN, yuan, fen);
    } else {
        lv_obj_set_style_text_color(cart_total_label, lv_palette_main(LV_PALETTE_GREY), 0);
        snprintf(total_buf, sizeof(total_buf), CN_CART_TOTAL "0.00 " CN_YUAN);
    }
    lv_label_set_text(cart_total_label, total_buf);

    // 联动刷新优惠券可用性（门槛基于当前总价）
    shop_ui_update_coupon_display();
}

/* 更新所有优惠券复选框的显示（剩余数量 + 门槛可用性） */
void shop_ui_update_coupon_display(void)
{
    // 优惠券门槛：[0]无优惠=0, [1]满20减5=20, [2]9折=0(无门槛), [3]满100打8折=100, [4]满200减50=200
    static const float disc_thresholds[5] = {0, 20.0f, 0, 100.0f, 200.0f};
    float cart_total = get_cart_total();

    for (int i = 0; i < 5; i++) {
        lv_obj_t * cb = discount_checkboxes[i];
        if (cb == NULL) continue;
        char buf[64];
        if (i == 0) {
            // "无优惠"始终可用（不显示剩余数量）
            lv_checkbox_set_text(cb, discount_base_texts[i]);
            lv_obj_clear_state(cb, LV_STATE_DISABLED);
            lv_obj_set_style_text_color(cb, lv_color_hex(0x333333), LV_PART_MAIN);
        } else {
            bool meets_threshold = (disc_thresholds[i] == 0 || cart_total >= disc_thresholds[i]);
            bool has_remaining   = (coupon_remaining[i] > 0);
            bool is_available    = meets_threshold && has_remaining;

            snprintf(buf, sizeof(buf), CN_COUPON_REMAINING, discount_base_texts[i], coupon_remaining[i]);
            lv_checkbox_set_text(cb, buf);

            if (is_available) {
                // 满足门槛且有余量：绿色可选中
                lv_obj_clear_state(cb, LV_STATE_DISABLED);
                lv_obj_set_style_text_color(cb, lv_color_hex(0x00C800), LV_PART_MAIN);
            } else if (!has_remaining) {
                // 已用完：红色禁用
                lv_obj_add_state(cb, LV_STATE_DISABLED);
                lv_obj_set_style_text_color(cb, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
            } else {
                // 有余量但不满足门槛：灰色禁用
                lv_obj_add_state(cb, LV_STATE_DISABLED);
                lv_obj_set_style_text_color(cb, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
            }
        }
    }

    // 当前选中的优惠券若不再可用，自动回退到"无优惠"
    if (current_discount > 0 && current_discount < 5) {
        bool meets = (disc_thresholds[current_discount] == 0 || cart_total >= disc_thresholds[current_discount]);
        if (!meets || coupon_remaining[current_discount] <= 0) {
            lv_obj_clear_state(discount_checkboxes[current_discount], LV_STATE_CHECKED);
            lv_obj_add_state(discount_checkboxes[0], LV_STATE_CHECKED);
            current_discount = 0;
        }
    }
}

/* 优惠券管理主页按钮回调 */
static void coupon_mgmt_btn_home_cb(lv_event_t * e)
{
    (void)e;
    show_coupon_mgmt_screen();
}

/* 更新优惠券管理页面的数量显示 */
static void shop_ui_update_coupon_mgmt_display(void)
{
    for (int i = 0; i < 4; i++) {
        lv_obj_t * lbl = coupon_count_labels[i];
        if (lbl == NULL) continue;
        int idx = i + 1;  // coupon_remaining index (1-4)
        lv_label_set_text_fmt(lbl, CN_CUR_QTY_FMT, coupon_remaining[idx]);
    }
}

/* +/- 按钮回调：修改优惠券数量 */
static void coupon_mgmt_plus_minus_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    lv_obj_t * btn = lv_event_get_target(e);
    int coupon_idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (coupon_idx < 1 || coupon_idx > 4) return;

    // 通过按钮的第一个子对象（标签）的文本判断是 + 还是 -
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
    coupon_config_save();  // 持久化到SD卡
}

/* 重置按钮回调：全部恢复为10张 */
static void coupon_mgmt_reset_cb(lv_event_t * e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    for (int i = 1; i <= 4; i++) {
        coupon_remaining[i] = 10;
    }

    shop_ui_update_coupon_mgmt_display();
    shop_ui_update_coupon_display();
    coupon_config_save();  // 持久化到SD卡
}

/* 关闭优惠券管理界面 */
void shop_ui_close_coupon_mgmt_screen(void)
{
    if (coupon_mgmt_screen) {
        coupon_mgmt_screen = NULL;
        for (int i = 0; i < 4; i++) {
            coupon_count_labels[i] = NULL;
        }
    }
}

/* 显示优惠券管理界面 */
void show_coupon_mgmt_screen(void)
{
    if (coupon_mgmt_screen) {
        lv_scr_load_anim(coupon_mgmt_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        shop_ui_update_coupon_mgmt_display();
        return;
    }

    // 创建管理界面屏幕
    coupon_mgmt_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(coupon_mgmt_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(coupon_mgmt_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(coupon_mgmt_screen, LV_OBJ_FLAG_SCROLLABLE);

    // 标题
    lv_obj_t * title = lv_label_create(coupon_mgmt_screen);
    lv_label_set_text(title, CN_COUPON_MGMT);
    lv_obj_set_style_text_font(title, &ziti_title, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_pos(title, 20, 15);

    // 返回主页按钮
    lv_obj_t * back_btn = lv_btn_create(coupon_mgmt_screen);
    lv_obj_set_size(back_btn, 100, 32);
    lv_obj_set_pos(back_btn, 904, 15);
    lv_obj_set_style_bg_color(back_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, CN_BTN_BACK);
    lv_obj_center(back_lbl);

    // 4种优惠券行（索引1-4对应 coupon_remaining[1]~[4]）
    for (int i = 0; i < 4; i++) {
        int coupon_idx = i + 1;
        int y = 110 + i * 90;

        // 行容器
        lv_obj_t * row = lv_obj_create(coupon_mgmt_screen);
        lv_obj_set_size(row, 880, 72);
        lv_obj_set_pos(row, 72, y);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        // 优惠券名称标签
        lv_obj_t * name_lbl = lv_label_create(row);
        lv_label_set_text(name_lbl, discount_base_texts[coupon_idx]);
        lv_obj_set_style_text_font(name_lbl, &ziti_title, 0);
        lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 20, 0);

        // 数量标签
        lv_obj_t * count_lbl = lv_label_create(row);
        coupon_count_labels[i] = count_lbl;
        lv_obj_set_style_text_font(count_lbl, &ziti_title, 0);
        lv_obj_set_style_text_color(count_lbl, lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_label_set_text_fmt(count_lbl, CN_CUR_QTY_FMT, coupon_remaining[coupon_idx]);
        lv_obj_align(count_lbl, LV_ALIGN_RIGHT_MID, -195, 0);

        // "-" 按钮
        lv_obj_t * minus_btn = lv_btn_create(row);
        lv_obj_set_size(minus_btn, 55, 42);
        lv_obj_align(minus_btn, LV_ALIGN_RIGHT_MID, -120, 0);
        lv_obj_set_style_bg_color(minus_btn, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_add_event_cb(minus_btn, coupon_mgmt_plus_minus_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)coupon_idx);
        lv_obj_t * minus_lbl = lv_label_create(minus_btn);
        lv_label_set_text(minus_lbl, "-");
        lv_obj_center(minus_lbl);

        // "+" 按钮
        lv_obj_t * plus_btn = lv_btn_create(row);
        lv_obj_set_size(plus_btn, 55, 42);
        lv_obj_align(plus_btn, LV_ALIGN_RIGHT_MID, -50, 0);
        lv_obj_set_style_bg_color(plus_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_add_event_cb(plus_btn, coupon_mgmt_plus_minus_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)coupon_idx);
        lv_obj_t * plus_lbl = lv_label_create(plus_btn);
        lv_label_set_text(plus_lbl, "+");
        lv_obj_center(plus_lbl);
    }

    // 底部重置按钮
    lv_obj_t * reset_btn = lv_btn_create(coupon_mgmt_screen);
    lv_obj_set_size(reset_btn, 200, 45);
    lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(reset_btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_add_event_cb(reset_btn, coupon_mgmt_reset_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, CN_RESET_DEFAULT);
    lv_obj_set_style_text_font(reset_lbl, &ziti, 0);
    lv_obj_center(reset_lbl);

    // 切换到管理界面
    lv_scr_load_anim(coupon_mgmt_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}
/* 初始化UI（显示主页） */
void shop_ui_init(void)
{
    show_home_screen();
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
    lv_obj_fade_in(mbox, 150, 0);  // 淡入动画
    
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
    lv_obj_fade_in(mbox, 150, 0);  // 淡入动画
    return mbox;
}

// ==================== 交易历史面板辅助 ====================

static lv_obj_t * hist_list  = NULL;    // 可滚动列表容器

static void hist_list_item_cb(lv_event_t * e);  // 前向声明
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

/* 关闭交易详情弹窗（淡出后删除遮罩层）*/
static void close_detail_del_cb(lv_anim_t * a)
{
    lv_obj_del((lv_obj_t *)a->var);
}

static void close_detail_popup(lv_event_t * e)
{
    lv_obj_t * overlay = (lv_obj_t *)lv_event_get_user_data(e);
    if(overlay == NULL) return;

    // 淡出动画，完成后自动删除
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, overlay);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 100);
    lv_anim_set_ready_cb(&a, close_detail_del_cb);
    lv_anim_start(&a);
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
    lv_obj_set_pos(popup, 262, 75);
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

    // 淡入动画
    lv_obj_fade_in(overlay, 150, 0);
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

    lv_obj_clean(hist_list);

    int total = (int)tx_log.count;

    if(total == 0) {
        lv_obj_t * lbl = lv_label_create(hist_list);
        lv_label_set_text(lbl, CN_TX_EMPTY);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(lbl, lv_pct(100));
        lv_obj_center(lbl);
    } else {
        lv_coord_t y = 2;
        for(int i = total - 1; i >= 0; i--) {
            transaction_t * tx = &tx_log.records[i];

            static char line_buf[128];
            if(tx->total_before_discount != tx->total_after_discount) {
                snprintf(line_buf, sizeof(line_buf),
                         "%s%lu | %d%s | %s: %.2f%s",
                         CN_TX_ID, (unsigned long)tx->id, (int)tx->item_count,
                         CN_TX_ITEMS, CN_TX_PAY,
                         tx->total_after_discount, CN_YUAN);
            } else {
                snprintf(line_buf, sizeof(line_buf),
                         "%s%lu | %d%s | %s: %.2f%s",
                         CN_TX_ID, (unsigned long)tx->id, (int)tx->item_count,
                         CN_TX_ITEMS, CN_TX_TOTAL,
                         tx->total_after_discount, CN_YUAN);
            }

            // 行容器（可点击，不滚动）
            lv_obj_t * row = lv_obj_create(hist_list);
            lv_obj_set_size(row, 994, 38);
            lv_obj_set_pos(row, 5, y);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_style_bg_color(row, lv_color_hex(0xF0F0F0), 0);
            lv_obj_set_style_radius(row, 6, 0);
            lv_obj_set_style_pad_all(row, 6, 0);
            lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_event_cb(row, hist_list_item_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

            lv_obj_t * lbl = lv_label_create(row);
            lv_label_set_text(lbl, line_buf);
            lv_obj_set_style_text_font(lbl, &ziti, 0);
            lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 6, 0);

            y += 42; // 38 + 4 gap
        }
    }
}

/* 关闭历史面板 */
void shop_ui_close_history_panel(void)
{
    if(history_screen) {
        history_screen = NULL;
        hist_list = NULL;
    }
}

/* 显示交易历史面板（独立全屏页面） */
void show_history_panel(void)
{
    if(history_screen) {
        lv_scr_load_anim(history_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        shop_ui_refresh_history_list();
        return;
    }

    // === 屏幕（禁止滚动，作为固定画布） ===
    history_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(history_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(history_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(history_screen, LV_OBJ_FLAG_SCROLLABLE);

    // === 顶部固定区域 ===
    lv_obj_t * main_title = lv_label_create(history_screen);
    lv_label_set_text(main_title, CN_TX_DETAIL);
    lv_obj_set_style_text_font(main_title, &ziti_title, 0);
    lv_obj_set_style_text_color(main_title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_pos(main_title, 20, 15);

    lv_obj_t * back_btn = lv_btn_create(history_screen);
    lv_obj_set_size(back_btn, 100, 32);
    lv_obj_set_pos(back_btn, 904, 15);
    lv_obj_set_style_bg_color(back_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, CN_BTN_BACK);
    lv_obj_center(back_lbl);

    static char max_buf[48];
    snprintf(max_buf, sizeof(max_buf), CN_MAX_RECORDS, MAX_TX_HISTORY);
    lv_obj_t * subtitle = lv_label_create(history_screen);
    lv_label_set_text(subtitle, max_buf);
    lv_obj_set_style_text_color(subtitle, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_pos(subtitle, 20, 55);

    // === 可滚动列表区域（普通容器，手动定位子行） ===
    hist_list = lv_obj_create(history_screen);
    lv_obj_set_size(hist_list, 1004, 470);
    lv_obj_set_pos(hist_list, 10, 80);
    lv_obj_set_style_border_width(hist_list, 0, 0);
    lv_obj_set_style_bg_opa(hist_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(hist_list, 0, 0);
    lv_obj_set_scrollbar_mode(hist_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(hist_list, LV_DIR_VER);
    lv_obj_clear_flag(hist_list, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    // === 底部固定区域 ===
    lv_obj_t * btn_clr = lv_btn_create(history_screen);
    lv_obj_set_size(btn_clr, 120, 36);
    lv_obj_align(btn_clr, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_clr, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_clr, hist_clear_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_clr = lv_label_create(btn_clr);
    lv_label_set_text(lbl_clr, CN_HISTORY_CLR);
    lv_obj_center(lbl_clr);

    shop_ui_refresh_history_list();
    lv_scr_load_anim(history_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}
