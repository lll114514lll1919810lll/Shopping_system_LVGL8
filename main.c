#include "drivers.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "shop_app.h" 

int main()
{
    sys_init();
    
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_60MHZ, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_bit_reset(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    
    // 初始化交易记录系统（从SD卡加载历史记录）
    tx_log_init();

    // 加载优惠券配置（从SD卡恢复优惠券数量）
    coupon_config_load();

    // 加载商品价格配置（从SD卡恢复自定义价格）
    price_config_load();

    shop_ui_init(); 

    while(1){
        delay_us(2000);
        lv_timer_handler();
    }
}
