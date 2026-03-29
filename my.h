#include "drivers.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"

extern lv_obj_t *btn_label1;
extern uint16_t btn2_click_count;
extern bool led2_state;
extern bool display1_state;
extern int16_t angle_counter;

extern lv_obj_t *textarea2;
extern lv_obj_t *kb;
extern lv_obj_t *label2;

void button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
		lv_obj_t *btn = lv_event_get_target(e);
		
    switch (code)
    {
        case LV_EVENT_PRESSED:
						gpio_bit_write(GPIOA , GPIO_PIN_1 , RESET);
						lv_label_set_text(btn_label1, "Red");
            break;
        case LV_EVENT_RELEASED:
        case LV_EVENT_PRESS_LOST:
            gpio_bit_write(GPIOA, GPIO_PIN_1, SET);
						lv_label_set_text(btn_label1, "Blue");
            break;
        default:
            break;
    }
}

void button_event_cb2(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code)
    {
        case LV_EVENT_VALUE_CHANGED:
						led2_state = !led2_state;
						gpio_bit_write(GPIOA, GPIO_PIN_2, led2_state ? RESET : SET);
						btn2_click_count++ ;
        default:
            break;
    }
}

void button_event_cb0(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    switch (code)
    {
        case LV_EVENT_VALUE_CHANGED:
						display1_state = !display1_state;
        default:
            break;
    }
}

void img_drag_event_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    lv_indev_t *indev = lv_indev_get_act();

    if (indev == NULL) return;

    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    lv_coord_t x = lv_obj_get_x_aligned(target);
    lv_coord_t y = lv_obj_get_y_aligned(target);

    x += vect.x;
    y += vect.y;

    lv_obj_set_pos(target, x, y);
}

void img2_move_rotate_timer_cb(lv_timer_t * timer)
{
	if(angle_counter < 2700){
    lv_obj_t * img2 = (lv_obj_t *)timer->user_data;

    lv_coord_t x = lv_obj_get_x(img2);
    lv_coord_t y = lv_obj_get_y(img2);

    x += 10;
    y += 1;
		lv_obj_set_pos(img2, x, y);
	
		angle_counter += 20;
		lv_img_set_pivot(img2, 180, 180);
		lv_img_set_angle(img2, angle_counter);
	}
	else
		return;
}

static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_keyboard_set_textarea(kb, textarea2);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void screen_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (!lv_obj_has_flag(kb, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void kb_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb_obj = lv_event_get_target(e);     // ????
    uint32_t btn_id;
    const char *btn_text;

    if (code == LV_EVENT_VALUE_CHANGED) {
        btn_id = lv_btnmatrix_get_selected_btn(kb_obj);
        btn_text = lv_keyboard_get_btn_text(kb_obj, btn_id);

        if (strcmp(btn_text, LV_SYMBOL_OK) == 0) {
            const char *ta_text = lv_textarea_get_text(textarea2);
            lv_label_set_text(label2, ta_text);
            lv_textarea_set_text(textarea2, "");
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(kb, NULL);
        }
				else if (strcmp(btn_text, LV_SYMBOL_KEYBOARD) == 0) {
            lv_keyboard_mode_t mode = lv_keyboard_get_mode(kb_obj);
            if (mode == LV_KEYBOARD_MODE_NUMBER) {
                lv_keyboard_set_mode(kb_obj, LV_KEYBOARD_MODE_TEXT_LOWER);
            } else {
                lv_keyboard_set_mode(kb_obj, LV_KEYBOARD_MODE_NUMBER);
            }
    }
  }
}