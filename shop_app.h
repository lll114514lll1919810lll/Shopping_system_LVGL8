#ifndef _SHOP_APP_H
#define _SHOP_APP_H

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- 常量和硬件 ---- */
#define MAX_PRODUCTS         6
#define MAX_CART_ITEMS       10
#define MAX_TX_HISTORY       30
#define MAX_TX_ITEMS_PER_TX  10
#define TX_LOG_FILE          "0:/transactions.csv"
#define COUPON_CONFIG_FILE   "0:/coupon.dat"
#define PRICE_CONFIG_FILE    "0:/price.dat"
#define ADMIN_PASSWORD       "123456"

/* 开发板 4 个 LED（PA0-PA3，低电平点亮） */
#define LED_GREEN   GPIO_PIN_0
#define LED_BLUE    GPIO_PIN_1
#define LED_YELLOW  GPIO_PIN_2
#define LED_RED     GPIO_PIN_3
#define LED_ALL     (LED_GREEN | LED_BLUE | LED_YELLOW | LED_RED)

#define led_on(pin)      gpio_bit_reset(GPIOA, (pin))
#define led_off(pin)     gpio_bit_set(GPIOA, (pin))
#define led_toggle(pin)  gpio_bit_toggle(GPIOA, (pin))
#define led_blink(pin, ms)    do { led_on(pin); delay_us((ms)*1000); led_off(pin); } while(0)
#define led_blink_n(pin, ms, n) do { for(int _i_=0; _i_<(n); _i_++) { led_on(pin); delay_us((ms)*1000); led_off(pin); if(_i_<(n)-1) delay_us((ms)*500); } } while(0)

/* ---- 数据结构 ---- */

typedef struct {
    uint8_t id;
    const char * name;
    uint32_t price;
    const char * unit;
    const char * img_path;
} product_t;

typedef struct {
    uint8_t product_id;
    float   quantity;
    float   subtotal;
} tx_item_t;

typedef enum {
    TX_DISC_NONE = 0,
    TX_DISC_FULL_REDUCTION,
    TX_DISC_PERCENT_OFF,
    TX_DISC_FULL_100_80PCT,
    TX_DISC_FULL_200_50
} tx_discount_type_t;

typedef struct {
    uint32_t          id;
    float             total_before_discount;
    float             total_after_discount;
    tx_discount_type_t discount_type;
    uint8_t           item_count;
    tx_item_t         items[MAX_TX_ITEMS_PER_TX];
} transaction_t;

typedef struct {
    transaction_t records[MAX_TX_HISTORY];
    uint8_t       count;
    uint32_t      next_id;
} transaction_log_t;

typedef enum {
    KB_MODE_NONE = 0,
    KB_MODE_ADD_TO_CART,
    KB_MODE_EDIT_QUANTITY,
} kb_input_mode_t;

/* ---- 全局变量 ---- */

extern product_t shop_products[MAX_PRODUCTS];
extern transaction_log_t tx_log;
extern lv_obj_t * cart_list;
extern lv_obj_t * input_ta;
extern lv_obj_t * num_kb;
extern lv_obj_t * label_full;
extern int coupon_remaining[5];
extern lv_obj_t * discount_checkboxes[5];
extern int current_discount;
extern kb_input_mode_t current_kb_mode;

extern const lv_font_t ziti;
extern const lv_font_t ziti_title;
extern const lv_font_t ziti_max;
extern lv_obj_t * price_input_ta;
extern lv_obj_t * price_num_kb;
extern lv_obj_t * price_label_full;
extern int price_edit_idx;
extern const uint32_t default_prices[MAX_PRODUCTS];

extern lv_obj_t * password_input_ta;
extern lv_obj_t * password_num_kb;
extern lv_obj_t * password_overlay;

/* ---- 函数声明 ---- */

void shop_ui_init(void);
void show_home_screen(void);
void show_shop_screen(void);

/* 购物界面回调 */
void product_btn_event_cb(lv_event_t * e);
void kb_event_cb(lv_event_t * e);
void checkout_btn_event_cb(lv_event_t * e);
void clear_btn_event_cb(lv_event_t * e);
void label_event_cb(lv_event_t *e);
void discount_cb_event_cb(lv_event_t * e);
void cart_list_btn_event_cb(lv_event_t* e);

/* 页面跳转回调 */
void shop_btn_cb(lv_event_t * e);
void history_btn_home_cb(lv_event_t * e);
void back_btn_cb(lv_event_t * e);
void shop_back_btn_cb(lv_event_t * e);
void password_kb_event_cb(lv_event_t * e);
void coupon_mgmt_btn_home_cb(lv_event_t * e);
void price_mgmt_btn_home_cb(lv_event_t * e);

/* 优惠券管理 */
void show_coupon_mgmt_screen(void);
void shop_ui_close_coupon_mgmt_screen(void);
void shop_ui_update_coupon_mgmt_display(void);
void shop_ui_update_coupon_display(void);
void coupon_mgmt_plus_minus_cb(lv_event_t * e);
void coupon_mgmt_reset_cb(lv_event_t * e);

/* 价格管理 */
void show_price_mgmt_screen(void);
void shop_ui_close_price_mgmt_screen(void);
void shop_ui_update_shop_prices(void);
void shop_ui_update_price_mgmt_display(void);
void price_mgmt_plus_minus_cb(lv_event_t * e);
void price_mgmt_reset_cb(lv_event_t * e);
void price_label_click_cb(lv_event_t * e);
void price_kb_event_cb(lv_event_t * e);
void price_overlay_click_cb(lv_event_t * e);

/* 键盘输入 */
void show_input_ui(void);
void hide_input_ui(void);
void show_price_input_ui(int prod_idx);
void hide_price_input_ui(void);
void show_password_ui(void);
void hide_password_ui(void);

/* 交易记录 */
void tx_log_init(void);
void tx_log_add(transaction_t * tx);
void tx_log_clear(void);
void tx_log_delete(uint8_t index);
int tx_log_save_to_sd(void);
int tx_log_load_from_sd(void);
void show_history_panel(void);
void history_btn_event_cb(lv_event_t * e);
void shop_ui_close_shop_screen(void);
void shop_ui_close_history_panel(void);
void shop_ui_refresh_history_list(void);
void shop_ui_show_tx_detail(transaction_t * tx, uint8_t tx_index);
void hist_clear_cb(lv_event_t * e);
void hist_list_item_cb(lv_event_t * e);

/* 配置持久化 */
void price_config_load(void);
void price_config_save(void);
void coupon_config_load(void);
void coupon_config_save(void);

/* UI 辅助 */
lv_obj_t * shop_ui_show_msgbox(const char * title, const char * message, const lv_color_t * txt_color);
void shop_ui_add_cart_item(const char * item_text, const char * product_name);
void shop_ui_show_checkout_result(const transaction_t * tx, float grand_total, float final_total, const char * discount_desc);
lv_obj_t * shop_ui_show_cart_action_menu(void);
lv_obj_t * shop_ui_get_cart_delete_btn(void);
lv_obj_t * shop_ui_get_cart_edit_btn(void);
void shop_ui_close_cart_menu(void);
void shop_ui_update_cart_total(void);

/* 底层接口 */
extern void * sdram_malloc(uint32_t size);
extern void read_file_to_array(const char *filename, void *array, uint32_t size);

#endif
