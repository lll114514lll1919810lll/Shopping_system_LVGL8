#include "drivers.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "my.h"

lv_obj_t *btn_label1;
uint16_t btn2_click_count = 0;
bool led2_state = false;
bool display1_state = true;
int16_t angle_counter = 0;

lv_obj_t *textarea2;
lv_obj_t *kb;
lv_obj_t *label2;

void Hang2Hang();

int main(){
		sys_init();
		
		rcu_periph_clock_enable(RCU_GPIOA);
		gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
		gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
		gpio_bit_reset(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
		
		lv_init();
		lv_port_disp_init();
		lv_port_indev_init();
		
	
		lv_obj_t *display1 = lv_obj_create(NULL);
		lv_obj_t *display2 = lv_obj_create(NULL);
		
	  //Hang2Hang();
		lv_obj_t *label = lv_label_create(display1);
    lv_label_set_text(label, "Li Yiming U202514651");
    lv_obj_set_pos(label, 5, 5);
	
	
	static lv_style_t style_btn_pressed;
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0xFF0000));
		
		
		lv_obj_t *bt1 = lv_btn_create(display1);
		lv_obj_set_pos(bt1, 50, 50);
		btn_label1 = lv_label_create(bt1);
    lv_label_set_text(btn_label1, "Blue");
    lv_obj_center(btn_label1);
		lv_obj_add_event_cb(bt1, button_event_cb, LV_EVENT_ALL, NULL);
		lv_obj_add_style(bt1, &style_btn_pressed, LV_STATE_PRESSED);
		
		lv_obj_t *bt2 = lv_btn_create(display1);
		lv_obj_set_pos(bt2, 200, 50);
		lv_obj_t *btn2_label = lv_label_create(bt2);
    lv_label_set_text(btn2_label, "0");
    lv_obj_center(btn2_label);
		lv_obj_add_flag(bt2, LV_OBJ_FLAG_CHECKABLE);
		lv_obj_add_event_cb(bt2, button_event_cb2, LV_EVENT_ALL, NULL);
		
		lv_obj_t *sw = lv_switch_create(display1);
		lv_obj_set_width(sw, 70);
		lv_obj_set_height(sw, 35);
		lv_obj_center(sw);
		
		lv_obj_t *bt0 = lv_btn_create(display1);
		lv_obj_set_size(bt0 , 65 , 35);
		lv_obj_set_pos(bt0, 880, 50);
		lv_obj_t *label0 = lv_label_create(bt0);
		lv_label_set_text(label0, "Toggle");
		lv_obj_center(label0);
		lv_obj_add_flag(bt0, LV_OBJ_FLAG_CHECKABLE);
		lv_obj_add_event_cb(bt0, button_event_cb0, LV_EVENT_ALL, NULL);
		
		lv_obj_t *bt00 = lv_btn_create(display2);
		lv_obj_set_size(bt00 , 65 , 35);
		lv_obj_set_pos(bt00, 880, 50);
		lv_obj_t *label00 = lv_label_create(bt00);
		lv_label_set_text(label00, "Toggle");
		lv_obj_center(label00);
		lv_obj_add_flag(bt00, LV_OBJ_FLAG_CHECKABLE);
		lv_obj_add_event_cb(bt00, button_event_cb0, LV_EVENT_ALL, NULL);
		
		label2 = lv_label_create(display2);
		lv_label_set_text(label2, "");
		textarea2 = lv_textarea_create(display2);
		lv_obj_center(textarea2);
		lv_coord_t tay = lv_obj_get_y(textarea2);
		tay -= 80;
		lv_obj_set_y(textarea2 , tay);
		kb = lv_keyboard_create(display2);
		lv_keyboard_set_textarea(kb , textarea2);
		lv_obj_add_event_cb(textarea2, ta_event_cb, LV_EVENT_CLICKED, NULL);
		lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
		lv_obj_add_event_cb(display2, screen_event_cb, LV_EVENT_CLICKED, NULL);

		lv_obj_t * img = lv_img_create(display1);
		uint8_t* image_buffer = sdram_malloc( 370 * 233 * 3 + 4 );
		read_file_to_array("0:/550W.bin", image_buffer,  370 * 233 * 3 + 4 );
		lv_img_dsc_t image_struct;
		image_struct.header.always_zero = 0;
		image_struct.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
		image_struct.header.w = 370;
		image_struct.header.h = 233;
		image_struct.header.reserved = 0;
		image_struct.data_size = 370 * 233 * 3;
		image_struct.data = image_buffer + 4;
		lv_img_set_src(img, &image_struct);
		
		lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_event_cb(img, img_drag_event_cb, LV_EVENT_PRESSING, NULL);
		
		lv_obj_t * img2 = lv_img_create(display2);
		uint8_t* image_buffer2 = sdram_malloc( 360 * 360 * 3 + 4 );
		read_file_to_array("0:/no.bin", image_buffer2,  360 * 360 * 3 + 4 );
		lv_img_dsc_t image_struct2;
		image_struct2.header.always_zero = 0;
		image_struct2.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
		image_struct2.header.w = 360;
		image_struct2.header.h = 360;
		image_struct2.header.reserved = 0;
		image_struct2.data_size = 360 * 360 * 3;
		image_struct2.data = image_buffer2 + 4;
		lv_img_set_src(img2, &image_struct2);
		lv_timer_t * timer = lv_timer_create(img2_move_rotate_timer_cb, 70, img2);
		
	while(1){
			
			delay_us(2000);
			lv_timer_handler();
			if(lv_obj_has_state(sw, LV_STATE_CHECKED))
				gpio_bit_write(GPIOA , GPIO_PIN_0 , RESET);
			else
				gpio_bit_write(GPIOA , GPIO_PIN_0 , SET);
			char text_buf[20];
			sprintf(text_buf, "%d", btn2_click_count);
			lv_label_set_text(btn2_label, text_buf);
			
			if(display1_state == true)
							lv_scr_load(display1);
				else
							lv_scr_load(display2);

	}
}


