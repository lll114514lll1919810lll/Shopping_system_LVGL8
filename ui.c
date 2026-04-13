#include "shop_app.h"

// ??????? (???:????5???)
product_t shop_products[5] = {
    {0, "?????", 8,  "?"},
    {1, "?????", 6,  "?"},
    {2, "??????", 12, "?"},
    {3, "?????", 3,  "?"},
    {4, "????",   3,  "?"}
};

// ???UI????
lv_obj_t * cart_list = NULL;
lv_obj_t * input_ta = NULL;
lv_obj_t * num_kb = NULL;

void shop_ui_init(void)
{
    lv_obj_t * scr = lv_scr_act(); // ????????

    // ==========================================
    // ?? 1:??????? (?? Flex ????????)
    // ==========================================
    lv_obj_t * product_panel = lv_obj_create(scr);
    lv_obj_set_size(product_panel, 500, 560);
    lv_obj_set_pos(product_panel, 20, 20);
    lv_obj_set_flex_flow(product_panel, LV_FLEX_FLOW_ROW_WRAP); // ????
    
    // ??? for ???5???????(????????!)
    for(int i = 0; i < 5; i++) {
        lv_obj_t * btn = lv_btn_create(product_panel);
        lv_obj_set_size(btn, 140, 80); // ??????
        
        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "%s\n%d?/%s", 
                              shop_products[i].name, 
                              shop_products[i].price, 
                              shop_products[i].unit);
        lv_obj_center(label);
        
        // ??:??????,???????????(&shop_products[i])???????!
        lv_obj_add_event_cb(btn, product_btn_event_cb, LV_EVENT_CLICKED, &shop_products[i]);
    }

    // ==========================================
    // ?? 2:?????????? (?? lv_list ??)
    // ==========================================
    cart_list = lv_list_create(scr);
    lv_obj_set_size(cart_list, 450, 560);
    lv_obj_set_pos(cart_list, 550, 20);
    
    // ?????
    lv_list_add_text(cart_list, "====== ????? ======");

    // ==========================================
    // ?? 3:?????????????? (????)
    // ==========================================
    // 1. ??????
    input_ta = lv_textarea_create(scr);
    lv_obj_set_size(input_ta, 300, 60);
    lv_obj_align(input_ta, LV_ALIGN_CENTER, 0, -150);
    lv_textarea_set_one_line(input_ta, true); // ??????
    lv_obj_add_flag(input_ta, LV_OBJ_FLAG_HIDDEN); // ????

    // 2. ??????
    num_kb = lv_keyboard_create(scr);
    lv_keyboard_set_mode(num_kb, LV_KEYBOARD_MODE_NUMBER); // ?????????
    lv_obj_set_size(num_kb, 400, 250);
    lv_obj_align(num_kb, LV_ALIGN_CENTER, 0, 50);
    
    // ????????
    lv_keyboard_set_textarea(num_kb, input_ta);
    lv_obj_add_flag(num_kb, LV_OBJ_FLAG_HIDDEN); // ????
    
    // ?????????(????“OK”??)
    lv_obj_add_event_cb(num_kb, kb_event_cb, LV_EVENT_ALL, NULL);
}