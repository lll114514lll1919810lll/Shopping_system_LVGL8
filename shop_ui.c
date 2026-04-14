#include "shop_app.h"

// SDRAM 模拟堆管理 (从 4MB 偏移处开始)
static uint32_t sdram_heap_ptr = 0xC0000000 + (1024 * 1024 * 4); 

void * sdram_malloc(uint32_t size) {
    void * p = (void *)sdram_heap_ptr;
    sdram_heap_ptr += size; 
    if(sdram_heap_ptr % 4 != 0) sdram_heap_ptr += (4 - (sdram_heap_ptr % 4));
    return p;
}

// 商品数据定义
product_t shop_products[5] = {
    {0, "Apple",      8,  "kg",     "0:/apple.bin"},
    {1, "Milk",       6,  "box",    "0:/milk.bin"},
    {2, "Bread",      12, "pack",   "0:/bread.bin"},
    {3, "Watermelon", 3,  "kg",     "0:/xigua.bin"},
    {4, "Cola",       3,  "bottle", "0:/cola.bin"}
};

lv_obj_t * cart_list = NULL;
lv_obj_t * input_ta = NULL;
lv_obj_t * num_kb = NULL;
lv_obj_t * label_full = NULL;
static lv_img_dsc_t img_dscs[5]; // 存储图片描述符

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
    for(int i = 0; i < 5; i++) {
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
        lv_obj_set_size(item_cont, 210, 300); 
        lv_obj_clear_flag(item_cont, LV_OBJ_FLAG_SCROLLABLE);

        // 创建图片
        lv_obj_t * img = lv_img_create(item_cont);
        lv_img_set_src(img, &img_dscs[i]);
        lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);

        // 创建文字标签
        lv_obj_t * label = lv_label_create(item_cont);
        lv_label_set_text_fmt(label, "%s\n#ff0000 %d RMB# / %s", 
                              shop_products[i].name, 
                              shop_products[i].price, 
                              shop_products[i].unit);
        lv_label_set_recolor(label, true); 
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

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
		lv_label_set_text(cart_title, "Checklist");
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
		lv_obj_add_event_cb(label_full, label_event_cb, LV_EVENT_CLICKED, NULL);

    // 5. 底部控制按钮
    lv_obj_t * btn_checkout = lv_btn_create(scr);
    lv_obj_set_size(btn_checkout, 110, 50);
    lv_obj_set_pos(btn_checkout, 540, 530);
    lv_obj_set_style_bg_color(btn_checkout, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_checkout, checkout_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_checkout = lv_label_create(btn_checkout);
    lv_label_set_text(lbl_checkout, "Checkout");
    lv_obj_center(lbl_checkout);

    lv_obj_t * btn_clear = lv_btn_create(scr);
    lv_obj_set_size(btn_clear, 110, 50);
    lv_obj_set_pos(btn_clear, 670, 530); 
    lv_obj_set_style_bg_color(btn_clear, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_clear, clear_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * lbl_clear = lv_label_create(btn_clear);
    lv_label_set_text(lbl_clear, "Clear");
    lv_obj_center(lbl_clear);

		//6.优惠
		lv_obj_t * discount_panel = lv_obj_create(scr);
		lv_obj_set_size(discount_panel, 200, 450);
		lv_obj_set_pos(discount_panel, 790, 10);
		lv_obj_set_flex_flow(discount_panel, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_style_pad_gap(discount_panel, 12, 0);
		lv_obj_set_style_pad_all(discount_panel, 10, 0);

			// 标题
		lv_obj_t * discount_title = lv_label_create(discount_panel);
		lv_label_set_text(discount_title, "Discounts");
		lv_obj_set_style_text_align(discount_title, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_width(discount_title, lv_pct(100));

		// 复选框选项（互斥组）
		static const char * discount_opts[] = {"No discounts", "When 20 minus 5", "90% price", NULL};
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
