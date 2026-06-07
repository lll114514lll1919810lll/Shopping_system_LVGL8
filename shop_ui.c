#include "shop_app.h"
#include "shop_chinese.h"
#include "gd32h7xx.h"
#include <string.h>


lv_obj_t * discount_checkboxes[5] = {NULL};
static const char * discount_base_texts[] = {
    CN_DISC_NONE, CN_DISC_FULL20_RED5, CN_DISC_90PCT, CN_DISC_FULL100_80PCT, CN_DISC_FULL200_RED50
};

#ifndef PC_SIMULATOR
static uint32_t sdram_heap_ptr = 0xC0000000 + (1024 * 1024 * 4);

void * sdram_malloc(uint32_t size) {
    void * p = (void *)sdram_heap_ptr;
    sdram_heap_ptr += size;
    if(sdram_heap_ptr % 4 != 0) sdram_heap_ptr += (4 - (sdram_heap_ptr % 4));
    return p;
}
#endif

product_t shop_products[MAX_PRODUCTS] = {
    {0, CN_APPLE,      800,  "kg",        "0:/apple.bin"},
    {1, CN_MILK,       600,  CN_BOX,      "0:/milk.bin"},
    {2, CN_BREAD,      1000, CN_PACK,     "0:/bread.bin"},
    {3, CN_WATERMELON, 300,  "kg",        "0:/watermelon.bin"},
    {4, CN_COLA,       300,  CN_BOTTLE,   "0:/cola.bin"},
    {5, CN_CHOCOLATE,  4000, CN_BOX,      "0:/chocolate.bin"}
};

lv_obj_t * cart_list = NULL;
lv_obj_t * input_ta = NULL;
lv_obj_t * num_kb = NULL;
lv_obj_t * label_full = NULL;
static lv_img_dsc_t img_dscs[MAX_PRODUCTS];

static lv_obj_t * home_screen = NULL;
static lv_obj_t * shop_screen = NULL;
static lv_obj_t * history_screen = NULL;
static lv_obj_t * coupon_mgmt_screen = NULL;
static lv_obj_t * coupon_count_labels[4] = {NULL};
static lv_obj_t * cart_total_label = NULL;
static lv_obj_t * price_mgmt_screen = NULL;
static lv_obj_t * statistics_screen = NULL;
static lv_obj_t * price_labels[6] = {NULL};
static lv_obj_t * shop_price_labels[MAX_PRODUCTS] = {NULL};

lv_obj_t * price_input_ta = NULL;
lv_obj_t * price_num_kb = NULL;
lv_obj_t * price_label_full = NULL;
int price_edit_idx = -1;

const uint32_t default_prices[MAX_PRODUCTS] = {800, 600, 1000, 300, 300, 4000};

static lv_img_dsc_t bg_img_dsc;
static uint8_t * bg_img_buffer = NULL;

lv_obj_t * password_input_ta = NULL;
lv_obj_t * password_num_kb = NULL;
lv_obj_t * password_overlay = NULL;

// 显示主界面
void show_home_screen(void)
{
    if (home_screen) {
        lv_scr_load_anim(home_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        return;
    }

    home_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(home_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(home_screen, LV_OPA_COVER, 0);

    if (bg_img_buffer == NULL) {
        uint32_t bg_data_size = 1024 * 600 * 2;
        uint32_t bg_file_size = bg_data_size + 4;
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
    lv_obj_move_background(bg_img);

    lv_obj_t * btn_shop = lv_btn_create(home_screen);
    lv_obj_set_size(btn_shop, 200, 80);
    lv_obj_align(btn_shop, LV_ALIGN_CENTER, -120, 80);
    lv_obj_set_style_bg_color(btn_shop, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_opa(btn_shop, LV_OPA_90, 0);
    lv_obj_add_event_cb(btn_shop, shop_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_shop = lv_label_create(btn_shop);
    lv_label_set_text(lbl_shop, CN_BTN_SHOP);
    lv_obj_set_style_text_font(lbl_shop, &ziti_max, 0);
    lv_obj_center(lbl_shop);

    lv_obj_t * btn_history = lv_btn_create(home_screen);
    lv_obj_set_size(btn_history, 200, 80);
    lv_obj_align(btn_history, LV_ALIGN_CENTER, 120, 80);
    lv_obj_set_style_bg_color(btn_history, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_opa(btn_history, LV_OPA_90, 0);
    lv_obj_add_event_cb(btn_history, history_btn_home_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_history = lv_label_create(btn_history);
    lv_label_set_text(lbl_history, CN_BTN_HISTORY);
    lv_obj_set_style_text_font(lbl_history, &ziti_max, 0);
    lv_obj_center(lbl_history);

    lv_obj_t * btn_price = lv_btn_create(home_screen);
    lv_obj_set_size(btn_price, 200, 80);
    lv_obj_align(btn_price, LV_ALIGN_CENTER, -120, 200);
    lv_obj_set_style_bg_color(btn_price, lv_palette_main(LV_PALETTE_TEAL), 0);
    lv_obj_set_style_opa(btn_price, LV_OPA_90, 0);
    lv_obj_add_event_cb(btn_price, price_mgmt_btn_home_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_price = lv_label_create(btn_price);
    lv_label_set_text(lbl_price, CN_PRICE_MGMT);
    lv_obj_set_style_text_font(lbl_price, &ziti_max, 0);
    lv_obj_center(lbl_price);

    lv_obj_t * btn_manage = lv_btn_create(home_screen);
    lv_obj_set_size(btn_manage, 200, 80);
    lv_obj_align(btn_manage, LV_ALIGN_CENTER, 120, 200);
    lv_obj_set_style_bg_color(btn_manage, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_set_style_opa(btn_manage, LV_OPA_90, 0);
    lv_obj_add_event_cb(btn_manage, coupon_mgmt_btn_home_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_manage = lv_label_create(btn_manage);
    lv_label_set_text(lbl_manage, CN_COUPON_MGMT);
    lv_obj_set_style_text_font(lbl_manage, &ziti_max, 0);
    lv_obj_center(lbl_manage);

    lv_scr_load_anim(home_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
}


// 显示购物界面
void show_shop_screen(void)
{
    if (shop_screen) {
        lv_scr_load_anim(shop_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);

        shop_ui_update_shop_prices();
        shop_ui_update_coupon_display();
        return;
    }

    shop_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(shop_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(shop_screen, LV_OPA_COVER, 0);

    uint32_t img_size_with_header = 200 * 200 * 2 + 4;
    uint32_t pure_data_size = 200 * 200 * 2;
    lv_obj_t * product_panel = lv_obj_create(shop_screen);
    lv_obj_set_size(product_panel, 520, 580);
    lv_obj_set_pos(product_panel, 10, 10);
    lv_obj_set_flex_flow(product_panel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(product_panel, 15, 0);

    // 加载商品图片并创建商品展示项
    for(int i = 0; i < MAX_PRODUCTS; i++) {
        uint8_t * image_buffer = (uint8_t *)sdram_malloc(img_size_with_header);
        read_file_to_array(shop_products[i].img_path, image_buffer, img_size_with_header);

        img_dscs[i].header.always_zero = 0;
        img_dscs[i].header.cf = LV_IMG_CF_TRUE_COLOR;
        img_dscs[i].header.w = 200;
        img_dscs[i].header.h = 200;
        img_dscs[i].data_size = pure_data_size;
        img_dscs[i].data = image_buffer + 4;

        lv_obj_t * item_cont = lv_obj_create(product_panel);
        lv_obj_set_size(item_cont, 220, 280);
        lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_pad_all(item_cont, 2, 0);
        lv_obj_set_style_pad_top(item_cont, 0, 0);

        lv_obj_t * img = lv_img_create(item_cont);
        lv_img_set_src(img, &img_dscs[i]);
        lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t * name_label = lv_label_create(item_cont);
        lv_label_set_text(name_label, shop_products[i].name);
        lv_obj_set_style_text_font(name_label, &ziti_title, 0);
        lv_obj_align(name_label, LV_ALIGN_TOP_MID, 0, 208);
        lv_obj_set_style_text_align(name_label, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_t * price_label = lv_label_create(item_cont);
        shop_price_labels[i] = price_label;
        lv_label_set_text_fmt(price_label, "#ff0000 %d.%02d " CN_YUAN "# / %s",
                              shop_products[i].price / 100, shop_products[i].price % 100,
                              shop_products[i].unit);
        lv_label_set_recolor(price_label, true);
        lv_obj_set_style_text_font(price_label, &ziti, 0);
        lv_obj_align(price_label, LV_ALIGN_TOP_MID, 0, 245);
        lv_obj_set_style_text_align(price_label, LV_TEXT_ALIGN_CENTER, 0);

        lv_obj_add_flag(item_cont, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(item_cont, product_btn_event_cb, LV_EVENT_CLICKED, &shop_products[i]);
    }

    cart_list = lv_list_create(shop_screen);
    lv_obj_set_size(cart_list, 240, 425);
    lv_obj_set_pos(cart_list, 540, 10);
    lv_obj_set_style_pad_gap(cart_list, 12, 0);
    lv_obj_set_style_pad_all(cart_list, 10, 0);

    lv_obj_t * cart_title = lv_label_create(cart_list);
    lv_label_set_text(cart_title, CN_CART_TITLE);
    lv_obj_set_style_text_font(cart_title, &ziti_title, 0);
    lv_obj_set_style_text_align(cart_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(cart_title, lv_pct(100));

    cart_total_label = lv_label_create(shop_screen);
    lv_obj_set_pos(cart_total_label, 550, 440);
    lv_obj_set_width(cart_total_label, 220);
    lv_obj_set_style_text_font(cart_total_label, &ziti_title, 0);
    lv_obj_set_style_text_align(cart_total_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(cart_total_label, CN_CART_TOTAL "0.00 " CN_YUAN_BEFORE);

    input_ta = lv_textarea_create(shop_screen);
    lv_obj_set_size(input_ta, 300, 50);
    lv_obj_align(input_ta, LV_ALIGN_CENTER, 0, -100);
    lv_textarea_set_one_line(input_ta, true);
    lv_obj_set_style_text_font(input_ta, &ziti, 0);
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
    lv_obj_add_flag(label_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(label_full, label_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(label_full, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(label_full, LV_OPA_60, 0);

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

    lv_obj_t * btn_back = lv_btn_create(shop_screen);
    lv_obj_set_size(btn_back, 110, 50);
    lv_obj_set_pos(btn_back, 900, 530);
    lv_obj_set_style_bg_color(btn_back, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(btn_back, shop_back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, CN_BTN_BACK);
    lv_obj_center(lbl_back);

    password_input_ta = lv_textarea_create(shop_screen);
    lv_obj_set_size(password_input_ta, 300, 50);
    lv_obj_align(password_input_ta, LV_ALIGN_CENTER, 0, -100);
    lv_textarea_set_one_line(password_input_ta, true);
    lv_textarea_set_password_mode(password_input_ta, true);
    lv_textarea_set_max_length(password_input_ta, 16);
    lv_obj_set_style_text_font(password_input_ta, &ziti, 0);
    lv_obj_add_flag(password_input_ta, LV_OBJ_FLAG_HIDDEN);

    password_num_kb = lv_keyboard_create(shop_screen);
    lv_keyboard_set_mode(password_num_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_flag(password_num_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(password_num_kb, password_kb_event_cb, LV_EVENT_ALL, NULL);

    password_overlay = lv_label_create(shop_screen);
    lv_label_set_text(password_overlay, "");
    lv_obj_set_size(password_overlay, 1024, 600);
    lv_obj_move_background(password_overlay);
    lv_obj_add_flag(password_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(password_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(password_overlay, label_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(password_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(password_overlay, LV_OPA_60, 0);

    lv_obj_t * discount_panel = lv_obj_create(shop_screen);
    lv_obj_set_size(discount_panel, 220, 450);
    lv_obj_set_pos(discount_panel, 790, 10);
    lv_obj_set_flex_flow(discount_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(discount_panel, 12, 0);
    lv_obj_set_style_pad_all(discount_panel, 10, 0);

    lv_obj_t * discount_title = lv_label_create(discount_panel);
    lv_label_set_text(discount_title, CN_DISCOUNTS);
    lv_obj_set_style_text_font(discount_title, &ziti_title, 0);
    lv_obj_set_style_text_align(discount_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(discount_title, lv_pct(100));

    // 创建优惠券复选框
    for (int i = 0; i < 5; i++) {
        lv_obj_t * cb = lv_checkbox_create(discount_panel);
        discount_checkboxes[i] = cb;
        if (i == 0) {
            lv_checkbox_set_text(cb, discount_base_texts[i]);
        } else {
            char buf[64];
            snprintf(buf, sizeof(buf), CN_COUPON_REMAINING, discount_base_texts[i], coupon_remaining[i]);
            lv_checkbox_set_text(cb, buf);
            lv_obj_set_style_text_color(cb,
                coupon_remaining[i] > 0 ? lv_color_hex(0x00C800) : lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
            if (coupon_remaining[i] <= 0) {
                lv_obj_add_state(cb, LV_STATE_DISABLED);
            }
        }
        lv_obj_add_event_cb(cb, discount_cb_event_cb, LV_EVENT_VALUE_CHANGED, (void *)(uintptr_t)i);
    }

    if (current_discount > 0 && coupon_remaining[current_discount] > 0) {
        lv_obj_clear_state(discount_checkboxes[0], LV_STATE_CHECKED);
        lv_obj_add_state(discount_checkboxes[current_discount], LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(discount_checkboxes[0], LV_STATE_CHECKED);
        current_discount = 0;
    }

    shop_ui_update_coupon_display();

    lv_scr_load_anim(shop_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

// 关闭购物界面
void shop_ui_close_shop_screen(void)
{
    if(shop_screen) {
        shop_screen = NULL;
        cart_list = NULL;
        input_ta = NULL;
        num_kb = NULL;
        label_full = NULL;
        cart_total_label = NULL;
        password_input_ta = NULL;
        password_num_kb = NULL;
        password_overlay = NULL;
    }
}

// 计算购物车商品总价
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

// 刷新购物车底部总价
void shop_ui_update_cart_total(void)
{
    if(cart_total_label == NULL) return;

    float total = get_cart_total();

    static char total_buf[64];
    if(total > 0.0f) {
        lv_obj_set_style_text_color(cart_total_label, lv_palette_main(LV_PALETTE_RED), 0);
        int yuan = (int)total;
        int fen  = (int)((total - (float)yuan) * 100.0f + 0.5f);
        snprintf(total_buf, sizeof(total_buf), CN_CART_TOTAL "%d.%02d " CN_YUAN_BEFORE, yuan, fen);
    } else {
        lv_obj_set_style_text_color(cart_total_label, lv_palette_main(LV_PALETTE_GREY), 0);
        snprintf(total_buf, sizeof(total_buf), CN_CART_TOTAL "0.00 " CN_YUAN_BEFORE);
    }
    lv_label_set_text(cart_total_label, total_buf);

    shop_ui_update_coupon_display();
}

// 刷新优惠券复选框状态
void shop_ui_update_coupon_display(void)
{
    static const float disc_thresholds[5] = {0, 20.0f, 0, 100.0f, 200.0f};
    float cart_total = get_cart_total();

    for (int i = 0; i < 5; i++) {
        lv_obj_t * cb = discount_checkboxes[i];
        if (cb == NULL) continue;
        char buf[64];
        if (i == 0) {
            lv_checkbox_set_text(cb, discount_base_texts[i]);
            lv_obj_clear_state(cb, LV_STATE_DISABLED);
            lv_obj_set_style_text_color(cb, lv_color_hex(0x333333), LV_PART_MAIN);
        } else {
            int meets_threshold = (disc_thresholds[i] == 0 || cart_total >= disc_thresholds[i]);
            int has_remaining   = (coupon_remaining[i] > 0);
            int is_available    = meets_threshold && has_remaining;

            snprintf(buf, sizeof(buf), CN_COUPON_REMAINING, discount_base_texts[i], coupon_remaining[i]);
            lv_checkbox_set_text(cb, buf);

            if (is_available) {
                lv_obj_clear_state(cb, LV_STATE_DISABLED);
                lv_obj_set_style_text_color(cb, lv_color_hex(0x00C800), LV_PART_MAIN);
            } else if (!has_remaining) {
                lv_obj_add_state(cb, LV_STATE_DISABLED);
                lv_obj_set_style_text_color(cb, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
            } else {
                lv_obj_add_state(cb, LV_STATE_DISABLED);
                lv_obj_set_style_text_color(cb, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
            }
        }
    }

    if (current_discount > 0 && current_discount < 5) {
        int meets = (disc_thresholds[current_discount] == 0 || cart_total >= disc_thresholds[current_discount]);
        if (!meets || coupon_remaining[current_discount] <= 0) {
            lv_obj_clear_state(discount_checkboxes[current_discount], LV_STATE_CHECKED);
            lv_obj_add_state(discount_checkboxes[0], LV_STATE_CHECKED);
            current_discount = 0;
        }
    }
}

// 更新优惠券管理页面数量显示
void shop_ui_update_coupon_mgmt_display(void)
{
    for (int i = 0; i < 4; i++) {
        lv_obj_t * lbl = coupon_count_labels[i];
        if (lbl == NULL) continue;
        int idx = i + 1;
        lv_label_set_text_fmt(lbl, CN_CUR_QTY_FMT, coupon_remaining[idx]);
    }
}

// 关闭优惠券管理界面
void shop_ui_close_coupon_mgmt_screen(void)
{
    if (coupon_mgmt_screen) {
        coupon_mgmt_screen = NULL;
        for (int i = 0; i < 4; i++) {
            coupon_count_labels[i] = NULL;
        }
    }
}

// 显示优惠券管理界面
void show_coupon_mgmt_screen(void)
{
    if (coupon_mgmt_screen) {
        lv_scr_load_anim(coupon_mgmt_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        shop_ui_update_coupon_mgmt_display();
        return;
    }

    coupon_mgmt_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(coupon_mgmt_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(coupon_mgmt_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(coupon_mgmt_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title = lv_label_create(coupon_mgmt_screen);
    lv_label_set_text(title, CN_COUPON_MGMT);
    lv_obj_set_style_text_font(title, &ziti_title, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_pos(title, 20, 15);

    lv_obj_t * back_btn = lv_btn_create(coupon_mgmt_screen);
    lv_obj_set_size(back_btn, 100, 32);
    lv_obj_set_pos(back_btn, 904, 15);
    lv_obj_set_style_bg_color(back_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, CN_BTN_BACK);
    lv_obj_center(back_lbl);

    // 创建优惠券管理行
    for (int i = 0; i < 4; i++) {
        int coupon_idx = i + 1;
        int y = 110 + i * 90;

        lv_obj_t * row = lv_obj_create(coupon_mgmt_screen);
        lv_obj_set_size(row, 880, 72);
        lv_obj_set_pos(row, 72, y);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t * name_lbl = lv_label_create(row);
        lv_label_set_text(name_lbl, discount_base_texts[coupon_idx]);
        lv_obj_set_style_text_font(name_lbl, &ziti_title, 0);
        lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 20, 0);

        lv_obj_t * count_lbl = lv_label_create(row);
        coupon_count_labels[i] = count_lbl;
        lv_obj_set_style_text_font(count_lbl, &ziti_title, 0);
        lv_obj_set_style_text_color(count_lbl, lv_palette_main(LV_PALETTE_BLUE), 0);
        lv_label_set_text_fmt(count_lbl, CN_CUR_QTY_FMT, coupon_remaining[coupon_idx]);
        lv_obj_align(count_lbl, LV_ALIGN_RIGHT_MID, -195, 0);

        lv_obj_t * minus_btn = lv_btn_create(row);
        lv_obj_set_size(minus_btn, 55, 42);
        lv_obj_align(minus_btn, LV_ALIGN_RIGHT_MID, -120, 0);
        lv_obj_set_style_bg_color(minus_btn, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_add_event_cb(minus_btn, coupon_mgmt_plus_minus_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)coupon_idx);
        lv_obj_t * minus_lbl = lv_label_create(minus_btn);
        lv_label_set_text(minus_lbl, "-");
        lv_obj_center(minus_lbl);

        lv_obj_t * plus_btn = lv_btn_create(row);
        lv_obj_set_size(plus_btn, 55, 42);
        lv_obj_align(plus_btn, LV_ALIGN_RIGHT_MID, -50, 0);
        lv_obj_set_style_bg_color(plus_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_add_event_cb(plus_btn, coupon_mgmt_plus_minus_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)coupon_idx);
        lv_obj_t * plus_lbl = lv_label_create(plus_btn);
        lv_label_set_text(plus_lbl, "+");
        lv_obj_center(plus_lbl);
    }

    lv_obj_t * reset_btn = lv_btn_create(coupon_mgmt_screen);
    lv_obj_set_size(reset_btn, 200, 45);
    lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(reset_btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_add_event_cb(reset_btn, coupon_mgmt_reset_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, CN_RESET_DEFAULT);
    lv_obj_set_style_text_font(reset_lbl, &ziti, 0);
    lv_obj_center(reset_lbl);

    lv_scr_load_anim(coupon_mgmt_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

// 显示密码输入界面
void show_password_ui(void)
{
    if (password_input_ta == NULL || password_num_kb == NULL || password_overlay == NULL) return;

    lv_textarea_set_text(password_input_ta, "");
    lv_textarea_set_placeholder_text(password_input_ta, CN_PASSWORD_PLACEHOLDER);
    lv_keyboard_set_textarea(password_num_kb, password_input_ta);

    lv_obj_clear_flag(password_input_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(password_num_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(password_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(password_overlay);
    lv_obj_move_foreground(password_num_kb);
    lv_obj_move_foreground(password_input_ta);
}

// 隐藏密码输入界面
void hide_password_ui(void)
{
    if (password_input_ta != NULL) {
        lv_obj_add_flag(password_input_ta, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(password_input_ta, "");
    }
    if (password_num_kb != NULL) {
        lv_obj_add_flag(password_num_kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (password_overlay != NULL) {
        lv_obj_add_flag(password_overlay, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(password_overlay);
    }
}

// 初始化UI
void shop_ui_init(void)
{
    show_home_screen();
}

// 刷新价格管理页面的价格
void shop_ui_update_price_mgmt_display(void)
{
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        lv_obj_t * lbl = price_labels[i];
        if (lbl == NULL) continue;
        lv_label_set_text_fmt(lbl, CN_PRICE_CUR_FMT, shop_products[i].price / 100, shop_products[i].price % 100);
    }
}

// 刷新购物界面商品价格
void shop_ui_update_shop_prices(void)
{
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        lv_obj_t * lbl = shop_price_labels[i];
        if (lbl == NULL) continue;
        lv_label_set_text_fmt(lbl, "#ff0000 %d.%02d " CN_YUAN "# / %s",
                              shop_products[i].price / 100, shop_products[i].price % 100,
                              shop_products[i].unit);
        lv_label_set_recolor(lbl, true);
    }
}

// 显示价格输入键盘
void show_price_input_ui(int prod_idx)
{
    if (price_input_ta == NULL || price_num_kb == NULL || price_label_full == NULL) return;
    if (prod_idx < 0 || prod_idx >= MAX_PRODUCTS) return;

    price_edit_idx = prod_idx;

    static char prompt_str[64];
    snprintf(prompt_str, sizeof(prompt_str), CN_PRICE_NEW_FMT, shop_products[prod_idx].name);
    lv_textarea_set_placeholder_text(price_input_ta, prompt_str);

    char cur_price[16];
    snprintf(cur_price, sizeof(cur_price), "%d.%02d", shop_products[prod_idx].price / 100, shop_products[prod_idx].price % 100);
    lv_textarea_set_text(price_input_ta, cur_price);

    lv_keyboard_set_textarea(price_num_kb, price_input_ta);

    lv_obj_clear_flag(price_input_ta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(price_num_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(price_label_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(price_label_full);
    lv_obj_move_foreground(price_num_kb);
    lv_obj_move_foreground(price_input_ta);
}

// 隐藏价格输入键盘
void hide_price_input_ui(void)
{
    if (price_input_ta != NULL) {
        lv_obj_add_flag(price_input_ta, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(price_input_ta, "");
    }
    if (price_num_kb != NULL) {
        lv_obj_add_flag(price_num_kb, LV_OBJ_FLAG_HIDDEN);
    }
    if (price_label_full != NULL) {
        lv_obj_add_flag(price_label_full, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_background(price_label_full);
    }
    price_edit_idx = -1;
}

// 关闭价格管理界面
void shop_ui_close_price_mgmt_screen(void)
{
    if (price_mgmt_screen) {
        price_mgmt_screen = NULL;
        price_input_ta = NULL;
        price_num_kb = NULL;
        price_label_full = NULL;
        price_edit_idx = -1;
        for (int i = 0; i < MAX_PRODUCTS; i++) {
            price_labels[i] = NULL;
        }
    }
}

// 显示价格管理界面
void show_price_mgmt_screen(void)
{
    price_config_load();

    if (price_mgmt_screen) {
        lv_scr_load_anim(price_mgmt_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        shop_ui_update_price_mgmt_display();
        return;
    }

    price_mgmt_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(price_mgmt_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(price_mgmt_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(price_mgmt_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title = lv_label_create(price_mgmt_screen);
    lv_label_set_text(title, CN_PRICE_MGMT);
    lv_obj_set_style_text_font(title, &ziti_title, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_TEAL), 0);
    lv_obj_set_pos(title, 20, 15);

    lv_obj_t * back_btn = lv_btn_create(price_mgmt_screen);
    lv_obj_set_size(back_btn, 100, 32);
    lv_obj_set_pos(back_btn, 904, 15);
    lv_obj_set_style_bg_color(back_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, CN_BTN_BACK);
    lv_obj_center(back_lbl);

    // 创建价格管理行
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        int y = 80 + i * 78;

        lv_obj_t * row = lv_obj_create(price_mgmt_screen);
        lv_obj_set_size(row, 880, 62);
        lv_obj_set_pos(row, 72, y);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_radius(row, 8, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t * name_lbl = lv_label_create(row);
        lv_label_set_text(name_lbl, shop_products[i].name);
        lv_obj_set_style_text_font(name_lbl, &ziti_title, 0);
        lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, 20, 0);

        lv_obj_t * price_lbl = lv_label_create(row);
        price_labels[i] = price_lbl;
        lv_obj_set_style_text_font(price_lbl, &ziti_title, 0);
        lv_obj_set_style_text_color(price_lbl, lv_palette_main(LV_PALETTE_RED), 0);
        lv_label_set_text_fmt(price_lbl, CN_PRICE_CUR_FMT, shop_products[i].price / 100, shop_products[i].price % 100);
        lv_obj_align(price_lbl, LV_ALIGN_RIGHT_MID, -195, 0);
        lv_obj_add_flag(price_lbl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(price_lbl, price_label_click_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t * minus_btn = lv_btn_create(row);
        lv_obj_set_size(minus_btn, 55, 42);
        lv_obj_align(minus_btn, LV_ALIGN_RIGHT_MID, -120, 0);
        lv_obj_set_style_bg_color(minus_btn, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_add_event_cb(minus_btn, price_mgmt_plus_minus_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t * minus_lbl = lv_label_create(minus_btn);
        lv_label_set_text(minus_lbl, "-");
        lv_obj_center(minus_lbl);

        lv_obj_t * plus_btn = lv_btn_create(row);
        lv_obj_set_size(plus_btn, 55, 42);
        lv_obj_align(plus_btn, LV_ALIGN_RIGHT_MID, -50, 0);
        lv_obj_set_style_bg_color(plus_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_add_event_cb(plus_btn, price_mgmt_plus_minus_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        lv_obj_t * plus_lbl = lv_label_create(plus_btn);
        lv_label_set_text(plus_lbl, "+");
        lv_obj_center(plus_lbl);
    }

    lv_obj_t * reset_btn = lv_btn_create(price_mgmt_screen);
    lv_obj_set_size(reset_btn, 200, 45);
    lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(reset_btn, lv_palette_main(LV_PALETTE_ORANGE), 0);
    lv_obj_add_event_cb(reset_btn, price_mgmt_reset_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, CN_RESET_PRICE);
    lv_obj_set_style_text_font(reset_lbl, &ziti, 0);
    lv_obj_center(reset_lbl);

    price_input_ta = lv_textarea_create(price_mgmt_screen);
    lv_obj_set_size(price_input_ta, 300, 50);
    lv_obj_align(price_input_ta, LV_ALIGN_CENTER, 0, -100);
    lv_textarea_set_one_line(price_input_ta, true);
    lv_obj_set_style_text_font(price_input_ta, &ziti, 0);
    lv_obj_add_flag(price_input_ta, LV_OBJ_FLAG_HIDDEN);

    price_num_kb = lv_keyboard_create(price_mgmt_screen);
    lv_keyboard_set_mode(price_num_kb, LV_KEYBOARD_MODE_NUMBER);
    lv_obj_add_flag(price_num_kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(price_num_kb, price_kb_event_cb, LV_EVENT_ALL, NULL);

    price_label_full = lv_label_create(price_mgmt_screen);
    lv_label_set_text(price_label_full, "");
    lv_obj_set_size(price_label_full, 1024, 600);
    lv_obj_move_background(price_label_full);
    lv_obj_add_flag(price_label_full, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(price_label_full, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(price_label_full, price_overlay_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(price_label_full, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(price_label_full, LV_OPA_60, 0);

    lv_scr_load_anim(price_mgmt_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

// 弹窗相关

// 弹窗淡出后删除
static void popup_overlay_del_cb(lv_anim_t * a)
{
    lv_obj_del((lv_obj_t *)a->var);
}

// 防止同时弹出多个弹窗
static lv_obj_t * active_popup_overlay = NULL;

// 弹窗淡出关闭
static void popup_close_anim(lv_obj_t * overlay)
{
    if(overlay == NULL) return;
    if(active_popup_overlay == overlay) active_popup_overlay = NULL;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, overlay);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 120);
    lv_anim_set_ready_cb(&a, popup_overlay_del_cb);
    lv_anim_start(&a);
}

// 弹窗关闭按钮回调
static void popup_close_btn_cb(lv_event_t * e)
{
    lv_obj_t * overlay = (lv_obj_t *)lv_event_get_user_data(e);
    if(overlay) popup_close_anim(overlay);
}

// 点击遮罩关闭
static void popup_overlay_click_cb(lv_event_t * e)
{
    lv_obj_t * overlay = lv_event_get_target(e);
    popup_close_anim(overlay);
}

// 弹窗根据标题文字决定颜色和图标
lv_obj_t * shop_ui_show_msgbox(const char * title, const char * message, const lv_color_t * txt_color)
{
    if (active_popup_overlay) return active_popup_overlay;

    lv_color_t accent;
    const char * icon_symbol;

    // 根据标题类型选颜色
    if (title != NULL && strcmp(title, CN_ERROR) == 0) {
        accent = lv_palette_main(LV_PALETTE_RED);
        icon_symbol = LV_SYMBOL_CLOSE;
    } else if (title != NULL && strcmp(title, CN_HINT) == 0) {
        accent = lv_palette_main(LV_PALETTE_BLUE);
        icon_symbol = LV_SYMBOL_WARNING;
    } else if (title != NULL && strcmp(title, CN_SUCCESS) == 0) {
        accent = lv_palette_main(LV_PALETTE_GREEN);
        icon_symbol = LV_SYMBOL_OK;
    } else {
        accent = lv_palette_main(LV_PALETTE_GREY);
        icon_symbol = LV_SYMBOL_LIST;
    }

    lv_color_t accent_light = lv_color_mix(accent, lv_color_white(), 200);

    // 半透明遮罩
    lv_obj_t * overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, 1024, 600);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(overlay, popup_overlay_click_cb, LV_EVENT_CLICKED, NULL);
    active_popup_overlay = overlay;

    // 白色卡片
    lv_obj_t * card = lv_obj_create(overlay);
    lv_obj_set_size(card, 440, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(card, 180, 0);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_color(card, lv_color_black(), 0);
    lv_obj_set_style_shadow_ofs_x(card, 0, 0);
    lv_obj_set_style_shadow_ofs_y(card, 2, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_10, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_fade_in(card, 100, 0);

    // 标题栏
    lv_obj_t * title_bar = lv_obj_create(card);
    lv_obj_set_size(title_bar, LV_PCT(100), 52);
    lv_obj_set_style_bg_color(title_bar, accent_light, 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_pad_ver(title_bar, 0, 0);
    lv_obj_set_style_pad_hor(title_bar, 16, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 图标圆圈
    lv_obj_t * icon_circle = lv_obj_create(title_bar);
    lv_obj_set_size(icon_circle, 30, 30);
    lv_obj_align(icon_circle, LV_ALIGN_LEFT_MID, 16, 0);
    lv_obj_set_style_radius(icon_circle, 15, 0);
    lv_obj_set_style_bg_color(icon_circle, accent, 0);
    lv_obj_set_style_bg_opa(icon_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(icon_circle, 0, 0);
    lv_obj_clear_flag(icon_circle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * icon_lbl = lv_label_create(icon_circle);
    lv_label_set_text(icon_lbl, icon_symbol);
    lv_obj_center(icon_lbl);
    lv_obj_set_style_text_color(icon_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(icon_lbl, &ziti_title, 0);

    // 标题文字
    lv_obj_t * title_lbl = lv_label_create(title_bar);
    lv_label_set_text(title_lbl, title ? title : "");
    lv_obj_align(title_lbl, LV_ALIGN_LEFT_MID, 56, 0);
    lv_obj_set_style_text_font(title_lbl, &ziti_title, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x333333), 0);

    // 右上角X按钮
    lv_obj_t * close_btn = lv_btn_create(title_bar);
    lv_obj_set_size(close_btn, 30, 30);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_radius(close_btn, 15, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_0, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_add_event_cb(close_btn, popup_close_btn_cb, LV_EVENT_CLICKED, overlay);
    lv_obj_t * close_icon = lv_label_create(close_btn);
    lv_label_set_text(close_icon, LV_SYMBOL_CLOSE);
    lv_obj_center(close_icon);
    lv_obj_set_style_text_color(close_icon, lv_color_hex(0x888888), 0);

    // 消息内容区域
    lv_obj_t * body = lv_obj_create(card);
    lv_obj_set_size(body, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(body, 60, 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_0, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 20, 0);
    lv_obj_set_style_pad_top(body, 16, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * msg_lbl = lv_label_create(body);
    lv_label_set_text(msg_lbl, message ? message : "");
    lv_obj_set_width(msg_lbl, LV_PCT(100));
    lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(msg_lbl, &ziti, 0);
    if (txt_color != NULL) {
        lv_obj_set_style_text_color(msg_lbl, *txt_color, 0);
    } else {
        lv_obj_set_style_text_color(msg_lbl, lv_color_hex(0x444444), 0);
    }

    // 底部确定按钮
    lv_obj_t * footer = lv_obj_create(card);
    lv_obj_set_size(footer, LV_PCT(100), 56);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_0, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 12, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * ok_btn = lv_btn_create(footer);
    lv_obj_set_size(ok_btn, 120, 38);
    lv_obj_center(ok_btn);
    lv_obj_set_style_radius(ok_btn, 8, 0);
    lv_obj_set_style_bg_color(ok_btn, accent, 0);
    lv_obj_set_style_bg_opa(ok_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(ok_btn, 0, 0);
    lv_obj_set_style_border_width(ok_btn, 0, 0);
    lv_obj_add_event_cb(ok_btn, popup_close_btn_cb, LV_EVENT_CLICKED, overlay);
    lv_obj_t * ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, CN_OK);
    lv_obj_center(ok_lbl);
    lv_obj_set_style_text_font(ok_lbl, &ziti, 0);
    lv_obj_set_style_text_color(ok_lbl, lv_color_white(), 0);

    return overlay;
}

// 在购物车中找指定商品
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

// 往购物车加商品，已有同名就替换
void shop_ui_add_cart_item(const char * item_text, const char * product_name)
{
    if(cart_list == NULL || item_text == NULL || product_name == NULL) return;

    lv_obj_t * existing_btn = find_cart_item_by_name(product_name);
    if(existing_btn != NULL) {
        lv_obj_del(existing_btn);
    }

    lv_obj_t * btn = lv_list_add_btn(cart_list, LV_SYMBOL_OK, item_text);
    lv_obj_add_event_cb(btn, cart_list_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

// 结算成功弹窗
void shop_ui_show_checkout_result(const transaction_t * tx, float grand_total, float final_total, const char * discount_desc)
{
    if (active_popup_overlay) return;

    lv_color_t green       = lv_palette_main(LV_PALETTE_GREEN);
    lv_color_t green_light = lv_color_mix(green, lv_color_white(), 200);
    lv_color_t green_dark  = lv_palette_darken(LV_PALETTE_GREEN, 2);

    // 遮罩
    lv_obj_t * overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, 1024, 600);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(overlay, popup_overlay_click_cb, LV_EVENT_CLICKED, NULL);
    active_popup_overlay = overlay;

    // 卡片
    lv_obj_t * card = lv_obj_create(overlay);
    lv_obj_set_size(card, 480, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(card, 220, 0);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_color(card, lv_color_black(), 0);
    lv_obj_set_style_shadow_ofs_x(card, 0, 0);
    lv_obj_set_style_shadow_ofs_y(card, 2, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_10, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_fade_in(card, 100, 0);

    // 标题栏
    lv_obj_t * title_bar = lv_obj_create(card);
    lv_obj_set_size(title_bar, LV_PCT(100), 68);
    lv_obj_set_style_bg_color(title_bar, green_light, 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_pad_all(title_bar, 0, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 对勾圆圈
    lv_obj_t * check_circle = lv_obj_create(title_bar);
    lv_obj_set_size(check_circle, 40, 40);
    lv_obj_align(check_circle, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_radius(check_circle, 20, 0);
    lv_obj_set_style_bg_color(check_circle, green, 0);
    lv_obj_set_style_bg_opa(check_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(check_circle, 0, 0);
    lv_obj_clear_flag(check_circle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * check_icon = lv_label_create(check_circle);
    lv_label_set_text(check_icon, LV_SYMBOL_OK);
    lv_obj_center(check_icon);
    lv_obj_set_style_text_color(check_icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(check_icon, &ziti_title, 0);

    // 标题
    lv_obj_t * title_lbl = lv_label_create(title_bar);
    lv_label_set_text(title_lbl, CN_SUCCESS);
    lv_obj_align(title_lbl, LV_ALIGN_LEFT_MID, 72, 0);
    lv_obj_set_style_text_font(title_lbl, &ziti_title, 0);
    lv_obj_set_style_text_color(title_lbl, green_dark, 0);

    // X按钮
    lv_obj_t * close_btn = lv_btn_create(title_bar);
    lv_obj_set_size(close_btn, 30, 30);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, -12, 0);
    lv_obj_set_style_radius(close_btn, 15, 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_0, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_add_event_cb(close_btn, popup_close_btn_cb, LV_EVENT_CLICKED, overlay);
    lv_obj_t * close_icon = lv_label_create(close_btn);
    lv_label_set_text(close_icon, LV_SYMBOL_CLOSE);
    lv_obj_center(close_icon);
    lv_obj_set_style_text_color(close_icon, lv_color_hex(0x888888), 0);

    // 商品明细
    lv_obj_t * body = lv_obj_create(card);
    lv_obj_set_size(body, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(body, LV_OPA_0, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 16, 0);
    lv_obj_set_style_pad_ver(body, 8, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(body, 4, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    static char line_buf[128];
    for(uint8_t k = 0; k < tx->item_count; k++) {
        uint8_t pid = tx->items[k].product_id;
        if(pid < MAX_PRODUCTS) {
            snprintf(line_buf, sizeof(line_buf), "%s  x%.1f%s  =  %.2f " CN_YUAN,
                     shop_products[pid].name,
                     tx->items[k].quantity,
                     shop_products[pid].unit,
                     tx->items[k].subtotal);

            lv_obj_t * line_lbl = lv_label_create(body);
            lv_label_set_text(line_lbl, line_buf);
            lv_obj_set_style_text_font(line_lbl, &ziti, 0);
            lv_obj_set_style_text_color(line_lbl, lv_color_hex(0x555555), 0);
        }
    }

    // 分隔线
    lv_obj_t * sep = lv_obj_create(body);
    lv_obj_set_size(sep, LV_PCT(100), 1);
    lv_obj_set_style_bg_color(sep, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);

    // 金额汇总
    if(grand_total != final_total) {

        // 折前
        snprintf(line_buf, sizeof(line_buf), CN_BEFORE_DISC ": %.2f " CN_YUAN, grand_total);
        lv_obj_t * before_lbl = lv_label_create(body);
        lv_label_set_text(before_lbl, line_buf);
        lv_obj_set_style_text_font(before_lbl, &ziti, 0);
        lv_obj_set_style_text_color(before_lbl, lv_color_hex(0x888888), 0);

        // 折后
        snprintf(line_buf, sizeof(line_buf), CN_AFTER_DISC ": %.2f " CN_YUAN "%s",
                 final_total, discount_desc);
        lv_obj_t * after_lbl = lv_label_create(body);
        lv_label_set_text(after_lbl, line_buf);
        lv_obj_set_style_text_font(after_lbl, &ziti_title, 0);
        lv_obj_set_style_text_color(after_lbl, green_dark, 0);
    } else {
        snprintf(line_buf, sizeof(line_buf), CN_TOTAL ": %.2f " CN_YUAN, final_total);
        lv_obj_t * total_lbl = lv_label_create(body);
        lv_label_set_text(total_lbl, line_buf);
        lv_obj_set_style_text_font(total_lbl, &ziti_title, 0);
        lv_obj_set_style_text_color(total_lbl, green_dark, 0);
    }

    // 底部确定按钮
    lv_obj_t * footer = lv_obj_create(card);
    lv_obj_set_size(footer, LV_PCT(100), 56);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_0, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 12, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * ok_btn = lv_btn_create(footer);
    lv_obj_set_size(ok_btn, 140, 38);
    lv_obj_center(ok_btn);
    lv_obj_set_style_radius(ok_btn, 8, 0);
    lv_obj_set_style_bg_color(ok_btn, green, 0);
    lv_obj_set_style_bg_opa(ok_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(ok_btn, 0, 0);
    lv_obj_set_style_border_width(ok_btn, 0, 0);
    lv_obj_add_event_cb(ok_btn, popup_close_btn_cb, LV_EVENT_CLICKED, overlay);
    lv_obj_t * ok_lbl = lv_label_create(ok_btn);
    lv_label_set_text(ok_lbl, CN_OK);
    lv_obj_center(ok_lbl);
    lv_obj_set_style_text_font(ok_lbl, &ziti, 0);
    lv_obj_set_style_text_color(ok_lbl, lv_color_white(), 0);
}

// 购物车操作菜单上下文
static struct {
    lv_obj_t * overlay;
    lv_obj_t * delete_btn;
    lv_obj_t * edit_btn;
} cart_menu_ctx;

lv_obj_t * shop_ui_get_cart_delete_btn(void) { return cart_menu_ctx.delete_btn; }
lv_obj_t * shop_ui_get_cart_edit_btn(void)  { return cart_menu_ctx.edit_btn; }

// 关闭购物车操作菜单
void shop_ui_close_cart_menu(void)
{
    if(cart_menu_ctx.overlay) {
        popup_close_anim(cart_menu_ctx.overlay);
        cart_menu_ctx.overlay = NULL;
        cart_menu_ctx.delete_btn = NULL;
        cart_menu_ctx.edit_btn = NULL;
    }
}

// 购物车操作菜单弹窗
lv_obj_t * shop_ui_show_cart_action_menu(void)
{
    if (active_popup_overlay) return active_popup_overlay;

    lv_color_t red_accent  = lv_palette_main(LV_PALETTE_RED);
    lv_color_t blue_accent = lv_palette_main(LV_PALETTE_BLUE);

    // 遮罩
    lv_obj_t * overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, 1024, 600);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(overlay, popup_overlay_click_cb, LV_EVENT_CLICKED, NULL);
    active_popup_overlay = overlay;

    // 卡片
    lv_obj_t * card = lv_obj_create(overlay);
    lv_obj_set_size(card, 380, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(card, 200, 0);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 16, 0);
    lv_obj_set_style_shadow_color(card, lv_color_black(), 0);
    lv_obj_set_style_shadow_ofs_x(card, 0, 0);
    lv_obj_set_style_shadow_ofs_y(card, 4, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_fade_in(card, 150, 0);

    // 标题栏
    lv_obj_t * title_bar = lv_obj_create(card);
    lv_obj_set_size(title_bar, LV_PCT(100), 50);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_bar, 0, 0);
    lv_obj_set_style_pad_hor(title_bar, 16, 0);
    lv_obj_clear_flag(title_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title_lbl = lv_label_create(title_bar);
    lv_label_set_text(title_lbl, CN_CART_ACTION);
    lv_obj_align(title_lbl, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_font(title_lbl, &ziti_title, 0);
    lv_obj_set_style_text_color(title_lbl, lv_color_hex(0x333333), 0);

    // X按钮
    lv_obj_t * close_btn = lv_btn_create(title_bar);
    lv_obj_set_size(close_btn, 30, 30);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(close_btn, 15, 0);
    lv_obj_set_style_bg_opa(close_btn, LV_OPA_0, 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0);
    lv_obj_set_style_border_width(close_btn, 0, 0);
    lv_obj_add_event_cb(close_btn, popup_close_btn_cb, LV_EVENT_CLICKED, overlay);
    lv_obj_t * close_icon = lv_label_create(close_btn);
    lv_label_set_text(close_icon, LV_SYMBOL_CLOSE);
    lv_obj_center(close_icon);
    lv_obj_set_style_text_color(close_icon, lv_color_hex(0x888888), 0);

    // 按钮区域
    lv_obj_t * body = lv_obj_create(card);
    lv_obj_set_size(body, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(body, LV_OPA_0, 0);
    lv_obj_set_style_border_width(body, 0, 0);
    lv_obj_set_style_pad_all(body, 16, 0);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(body, 10, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    // 删除按钮
    lv_obj_t * del_btn = lv_btn_create(body);
    lv_obj_set_size(del_btn, LV_PCT(100), 48);
    lv_obj_set_style_radius(del_btn, 10, 0);
    lv_obj_set_style_bg_color(del_btn, lv_color_hex(0xFFF0F0), 0);
    lv_obj_set_style_bg_opa(del_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(del_btn, 1, 0);
    lv_obj_set_style_border_color(del_btn, lv_color_hex(0xFFCDD2), 0);
    lv_obj_set_style_shadow_width(del_btn, 0, 0);

    lv_obj_t * del_icon = lv_label_create(del_btn);
    lv_label_set_text(del_icon, LV_SYMBOL_TRASH);
    lv_obj_align(del_icon, LV_ALIGN_LEFT_MID, 16, 0);
    lv_obj_set_style_text_color(del_icon, red_accent, 0);
    lv_obj_set_style_text_font(del_icon, &ziti_title, 0);

    lv_obj_t * del_lbl = lv_label_create(del_btn);
    lv_label_set_text(del_lbl, CN_DELETE_ITEM);
    lv_obj_align(del_lbl, LV_ALIGN_LEFT_MID, 46, 0);
    lv_obj_set_style_text_font(del_lbl, &ziti, 0);
    lv_obj_set_style_text_color(del_lbl, red_accent, 0);

    // 修改数量按钮
    lv_obj_t * edit_btn = lv_btn_create(body);
    lv_obj_set_size(edit_btn, LV_PCT(100), 48);
    lv_obj_set_style_radius(edit_btn, 10, 0);
    lv_obj_set_style_bg_color(edit_btn, lv_color_hex(0xF0F5FF), 0);
    lv_obj_set_style_bg_opa(edit_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(edit_btn, 1, 0);
    lv_obj_set_style_border_color(edit_btn, lv_color_hex(0xBBDEFB), 0);
    lv_obj_set_style_shadow_width(edit_btn, 0, 0);

    lv_obj_t * edit_icon = lv_label_create(edit_btn);
    lv_label_set_text(edit_icon, LV_SYMBOL_EDIT);
    lv_obj_align(edit_icon, LV_ALIGN_LEFT_MID, 16, 0);
    lv_obj_set_style_text_color(edit_icon, blue_accent, 0);
    lv_obj_set_style_text_font(edit_icon, &ziti_title, 0);

    lv_obj_t * edit_lbl = lv_label_create(edit_btn);
    lv_label_set_text(edit_lbl, CN_EDIT_QTY);
    lv_obj_align(edit_lbl, LV_ALIGN_LEFT_MID, 46, 0);
    lv_obj_set_style_text_font(edit_lbl, &ziti, 0);
    lv_obj_set_style_text_color(edit_lbl, blue_accent, 0);

    // 取消按钮
    lv_obj_t * footer = lv_obj_create(card);
    lv_obj_set_size(footer, LV_PCT(100), 52);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_0, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 10, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * cancel_btn = lv_btn_create(footer);
    lv_obj_set_size(cancel_btn, 100, 34);
    lv_obj_center(cancel_btn);
    lv_obj_set_style_radius(cancel_btn, 8, 0);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xEEEEEE), 0);
    lv_obj_set_style_bg_opa(cancel_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(cancel_btn, 0, 0);
    lv_obj_set_style_border_width(cancel_btn, 0, 0);
    lv_obj_add_event_cb(cancel_btn, popup_close_btn_cb, LV_EVENT_CLICKED, overlay);
    lv_obj_t * cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, CN_CANCEL);
    lv_obj_center(cancel_lbl);
    lv_obj_set_style_text_font(cancel_lbl, &ziti, 0);
    lv_obj_set_style_text_color(cancel_lbl, lv_color_hex(0x666666), 0);

    cart_menu_ctx.overlay    = overlay;
    cart_menu_ctx.delete_btn = del_btn;
    cart_menu_ctx.edit_btn   = edit_btn;

    return overlay;
}

// 交易详情弹窗
static lv_obj_t * hist_list  = NULL;

static const char * get_discount_desc_str(tx_discount_type_t type)
{
    if(type == TX_DISC_FULL_REDUCTION)      return CN_DESC_DISC_FULL20;
    if(type == TX_DISC_PERCENT_OFF)         return CN_DESC_DISC_90PCT;
    if(type == TX_DISC_FULL_100_80PCT)      return CN_DESC_DISC_FULL100_80PCT;
    if(type == TX_DISC_FULL_200_50)         return CN_DESC_DISC_FULL200_RED50;
    return CN_DESC_DISC_NONE;
}

static void close_detail_del_cb(lv_anim_t * a)
{
    lv_obj_del((lv_obj_t *)a->var);
}

static void close_detail_popup(lv_event_t * e)
{
    lv_obj_t * overlay = (lv_obj_t *)lv_event_get_user_data(e);
    if(overlay == NULL) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, overlay);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 100);
    lv_anim_set_ready_cb(&a, close_detail_del_cb);
    lv_anim_start(&a);
}

// 删除记录按钮回调
static void detail_delete_btn_cb(lv_event_t * e)
{
    lv_obj_t * overlay = (lv_obj_t *)lv_event_get_user_data(e);
    if(overlay == NULL) return;

    uint8_t tx_index = (uint8_t)(uintptr_t)lv_obj_get_user_data(overlay);

    // 删除记录
    tx_log_delete(tx_index);

    // 关闭弹窗
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, overlay);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 100);
    lv_anim_set_ready_cb(&a, close_detail_del_cb);
    lv_anim_start(&a);

    // 刷新列表
    shop_ui_refresh_history_list();
}

// 统计页面
void show_statistics_screen(void)
{
    // 每次重建
    if (statistics_screen) {
        lv_obj_del(statistics_screen);
        statistics_screen = NULL;
    }

    // 计算每件商品的销售额总和
    float sales[MAX_PRODUCTS];
    memset(sales, 0, sizeof(sales));
    for (uint8_t i = 0; i < tx_log.count; i++) {
        transaction_t * tx = &tx_log.records[i];
        for (uint8_t j = 0; j < tx->item_count; j++) {
            uint8_t pid = tx->items[j].product_id;
            if (pid < MAX_PRODUCTS) {
                sales[pid] += tx->items[j].subtotal;
            }
        }
    }

    // 最大值100%高度
    float max_sales = 0.0f;
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        if (sales[i] > max_sales) max_sales = sales[i];
    }
    if (max_sales < 1.0f) max_sales = 1.0f;  // 避免除零

    // 柱状图参数
    const lv_coord_t chart_left  = 80;
    const lv_coord_t baseline_y  = 530;
    const lv_coord_t max_bar_h   = 420;
    const lv_coord_t bar_w       = 110;
    const lv_coord_t bar_gap     = 40;

    static const lv_palette_t bar_colors[MAX_PRODUCTS] = {
        LV_PALETTE_RED, LV_PALETTE_BLUE, LV_PALETTE_GREEN,
        LV_PALETTE_ORANGE, LV_PALETTE_PURPLE, LV_PALETTE_TEAL
    };

    statistics_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(statistics_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(statistics_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(statistics_screen, LV_OBJ_FLAG_SCROLLABLE);

    // 标题
    lv_obj_t * title = lv_label_create(statistics_screen);
    lv_label_set_text(title, CN_STATISTICS);
    lv_obj_set_style_text_font(title, &ziti_title, 0);
    lv_obj_set_style_text_color(title, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_pos(title, 20, 15);

    // 返回按钮
    lv_obj_t * back_btn = lv_btn_create(statistics_screen);
    lv_obj_set_size(back_btn, 100, 32);
    lv_obj_set_pos(back_btn, 904, 15);
    lv_obj_set_style_bg_color(back_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(back_btn, statistics_back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "\xe8\xbf\x94\xe5\x9b\x9e");
    lv_obj_center(back_lbl);

    // 底部基线
    lv_obj_t * base_line = lv_obj_create(statistics_screen);
    lv_obj_set_size(base_line, 880, 2);
    lv_obj_set_pos(base_line, chart_left - 10, baseline_y);
    lv_obj_set_style_bg_color(base_line, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_border_width(base_line, 0, 0);

    // 绘制6根柱子
    for (int i = 0; i < MAX_PRODUCTS; i++) {
        lv_coord_t bar_x = chart_left + i * (bar_w + bar_gap);
        lv_coord_t bar_h = (lv_coord_t)(sales[i] / max_sales * max_bar_h);
        if (bar_h < 1) bar_h = 1;  // 最小高度以免看不见

        // 柱子
        lv_obj_t * bar = lv_obj_create(statistics_screen);
        lv_obj_set_size(bar, bar_w, bar_h);
        lv_obj_set_pos(bar, bar_x, baseline_y - bar_h);
        lv_obj_set_style_bg_color(bar, lv_palette_main(bar_colors[i]), 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 4, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

        // 金额标签
        lv_obj_t * val_lbl = lv_label_create(statistics_screen);
        int int_part = (int)sales[i];
        int frac_part = (int)((sales[i] - int_part) * 100 + 0.5f);
        lv_label_set_text_fmt(val_lbl, "%d.%02d", int_part, frac_part);
        lv_obj_set_style_text_font(val_lbl, &ziti, 0);
        lv_obj_set_style_text_color(val_lbl, lv_palette_main(bar_colors[i]), 0);
        lv_obj_set_pos(val_lbl, bar_x, baseline_y - bar_h - 22);

        // 商品名标签
        lv_obj_t * name_lbl = lv_label_create(statistics_screen);
        lv_label_set_text(name_lbl, shop_products[i].name);
        lv_obj_set_style_text_font(name_lbl, &ziti, 0);
        lv_obj_set_style_text_color(name_lbl, lv_color_black(), 0);
        lv_obj_set_pos(name_lbl, bar_x, baseline_y + 8);
    }

    lv_scr_load_anim(statistics_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

// 统计页返回历史（反方向动画）
void show_history_panel_back(void)
{
    if(history_screen) {
        lv_scr_load_anim(history_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        shop_ui_refresh_history_list();
        return;
    }
    show_history_panel();
}

// 交易详情弹窗
void shop_ui_show_tx_detail(transaction_t * tx, uint8_t tx_index)
{
    if(tx == NULL) return;

    // 遮罩
    lv_obj_t * overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, 1024, 600);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(overlay, close_detail_popup, LV_EVENT_CLICKED, overlay);
    lv_obj_set_user_data(overlay, (void *)(uintptr_t)tx_index);

    // 弹窗
    lv_obj_t * popup = lv_obj_create(overlay);
    lv_obj_set_size(popup, 500, 440);
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
    lv_obj_add_flag(popup, LV_OBJ_FLAG_CLICKABLE);

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

    // 商品明细
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

    // 底部按钮容器
    lv_obj_t * btn_cont = lv_obj_create(popup);
    lv_obj_set_width(btn_cont, lv_pct(100));
    lv_obj_set_height(btn_cont, 40);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(btn_cont, 0, 0);
    lv_obj_set_style_pad_gap(btn_cont, 10, 0);
    lv_obj_clear_flag(btn_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 删除按钮
    lv_obj_t * delete_btn = lv_btn_create(btn_cont);
    lv_obj_set_width(delete_btn, lv_pct(45));
    lv_obj_set_style_bg_color(delete_btn, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(delete_btn, detail_delete_btn_cb, LV_EVENT_CLICKED, overlay);
    lv_obj_t * del_lbl = lv_label_create(delete_btn);
    lv_label_set_text(del_lbl, CN_DELETE_RECORD);
    lv_obj_set_style_text_font(del_lbl, &ziti, 0);
    lv_obj_center(del_lbl);

    // 关闭按钮
    lv_obj_t * close_btn = lv_btn_create(btn_cont);
    lv_obj_set_width(close_btn, lv_pct(45));
    lv_obj_set_style_bg_color(close_btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(close_btn, close_detail_popup, LV_EVENT_CLICKED, overlay);
    lv_obj_t * close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, CN_OK);
    lv_obj_set_style_text_font(close_lbl, &ziti, 0);
    lv_obj_center(close_lbl);

    lv_obj_fade_in(overlay, 150, 0);
}

// 刷新历史记录列表
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

            y += 42;
        }
    }
}

// 关闭历史面板
void shop_ui_close_history_panel(void)
{
    if(history_screen) {
        history_screen = NULL;
        hist_list = NULL;
    }
}

// 显示交易历史页面
void show_history_panel(void)
{
    if(history_screen) {
        lv_scr_load_anim(history_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        shop_ui_refresh_history_list();
        return;
    }

    history_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(history_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(history_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(history_screen, LV_OBJ_FLAG_SCROLLABLE);

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

    hist_list = lv_obj_create(history_screen);
    lv_obj_set_size(hist_list, 1004, 470);
    lv_obj_set_pos(hist_list, 10, 80);
    lv_obj_set_style_border_width(hist_list, 0, 0);
    lv_obj_set_style_bg_opa(hist_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(hist_list, 0, 0);
    lv_obj_set_scrollbar_mode(hist_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(hist_list, LV_DIR_VER);
    lv_obj_clear_flag(hist_list, LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    lv_obj_t * btn_clr = lv_btn_create(history_screen);
    lv_obj_set_size(btn_clr, 120, 36);
    lv_obj_align(btn_clr, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(btn_clr, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_clr, hist_clear_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_clr = lv_label_create(btn_clr);
    lv_label_set_text(lbl_clr, CN_HISTORY_CLR);
    lv_obj_center(lbl_clr);

    // 统计按钮（右下角）
    lv_obj_t * btn_stats = lv_btn_create(history_screen);
    lv_obj_set_size(btn_stats, 120, 36);
    lv_obj_align(btn_stats, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_bg_color(btn_stats, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_stats, detail_statistics_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_stats = lv_label_create(btn_stats);
    lv_label_set_text(lbl_stats, CN_STATISTICS);
    lv_obj_set_style_text_font(lbl_stats, &ziti, 0);
    lv_obj_center(lbl_stats);

    shop_ui_refresh_history_list();
    lv_scr_load_anim(history_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}
