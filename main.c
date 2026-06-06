#include "drivers.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "shop_app.h"

int main()
{
    sys_init();
    
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_ALL);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, LED_ALL);
    gpio_bit_set(GPIOA, LED_ALL);   // 默认全部熄灭（高电平）
    
    // 开机灯效
    led_blink(LED_GREEN,  150); delay_us(80000);
    led_blink(LED_BLUE,   150); delay_us(80000);
    led_blink(LED_YELLOW, 150); delay_us(80000);
    led_blink(LED_RED,    150); delay_us(150000);
    led_on(LED_ALL);  delay_us(120000);
    led_off(LED_ALL); delay_us(80000);
    
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    
    led_blink_n(LED_BLUE, 60, 2);
    tx_log_init();

    led_blink_n(LED_BLUE, 60, 2);
    coupon_config_load();

    led_blink_n(LED_BLUE, 60, 2);
    price_config_load();

    shop_ui_init();
    
    led_blink(LED_GREEN, 100); delay_us(80000);
    led_blink(LED_GREEN, 100);

    while(1){
        delay_us(2000);
        lv_timer_handler();
    }
}

// C语言程序设计课程项目最终版
