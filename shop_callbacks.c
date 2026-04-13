#include "shop_app.h"

// 记录当前点击的是哪个商品，用于键盘确认时计算
static product_t * current_product = NULL;

// 引用在 ui.c 中定义的变量
extern lv_obj_t * cart_list;
extern lv_obj_t * input_ta;
extern lv_obj_t * num_kb;
extern lv_obj_t * label_full;

// --- A. 点击商品卡片的回调 ---
void product_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        product_t * p = (product_t *)lv_event_get_user_data(e);
        if(p == NULL || input_ta == NULL || num_kb == NULL) return;

        current_product = p; // 存入静态变量

        // 动态设置文本框提示语
        static char prompt_str[64];
        snprintf(prompt_str, sizeof(prompt_str), "Qty for [%s] (%s):", p->name, p->unit);
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
            float new_weight = (float)atof(input_str);
            if(new_weight <= 0) return; // 忽略无效输入

            float total_price = new_weight * (float)current_product->price;
            
            // 准备新的文字内容
            static char buf[128];
            snprintf(buf, sizeof(buf), "%s x%.1f %s = %.2f RMB", 
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
                // 如果找到了：直接更新这一行的文字
                // 在 LVGL 的 list 按钮中，文字存在于它内部的 label 子对象里
                lv_obj_t * label = lv_obj_get_child(existing_btn, -1); // 获取最后一个子对象，通常是 label
                if(label) {
                    lv_label_set_text(label, buf);
                }
            } else {
                // 如果没找到：新增一行按钮
                lv_list_add_btn(cart_list, LV_SYMBOL_OK, buf);
            }
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
        static const char * btns[] = {"OK", ""};
        lv_obj_t * mbox = lv_msgbox_create(NULL, "Hint", "Your cart is empty!", btns, true);
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

    // 3. 准备结果字符串 (放在 static 缓冲区防止局部变量销毁)
    static char result_buf[64];
    // 使用 %.2f 格式化总价
    snprintf(result_buf, sizeof(result_buf), "Total Amount: %.2f RMB", grand_total);

    // 4. 创建弹窗时直接传入结果字符串
    static const char * mbtns[] = {"Close", ""};
    // 注意：这里的 result_buf 代替了之前的 ""
    lv_obj_t * mbox = lv_msgbox_create(NULL, "Checkout Success", result_buf, mbtns, true);
    
    if(mbox) {
        lv_obj_center(mbox);
        // 如果需要给文字上色，在创建后再单独获取 label 处理
        lv_obj_t * mbox_txt = lv_msgbox_get_text(mbox);
        if(mbox_txt) {
            lv_obj_set_style_text_color(mbox_txt, lv_palette_main(LV_PALETTE_RED), 0);
        }
    }
}

// --- D. 清空回调 ---
void clear_btn_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED && cart_list != NULL) {
        lv_obj_clean(cart_list);
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

