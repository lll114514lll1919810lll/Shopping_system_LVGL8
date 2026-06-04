/**
 * @file main.c
 * @brief PC 模拟器入口
 *
 * 模拟原 main.c 的启动流程，用 SDL2 替代硬件外设。
 *
 * 原流程: sys_init → LED 上电自检 → lv_init → disp/indev init
 *         → tx_log/coupon/price load → shop_ui_init → 主循环
 */

#include <SDL.h>
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "drivers.h"
#include "shop_app.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("  Shopping System LVGL PC Simulator\n");
    printf("========================================\n\n");

    /* ========== 1. 初始化 SDL2 ========== */
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("[MAIN] SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    printf("[MAIN] SDL2 initialized\n");

    /* ========== 2. 模拟硬件初始化（全部桩实现） ========== */
    sys_init();

    /* 开机流水灯效果 */
    led_blink(LED_GREEN,  150); hal_delay_ms(80);
    led_blink(LED_BLUE,   150); hal_delay_ms(80);
    led_blink(LED_YELLOW, 150); hal_delay_ms(80);
    led_blink(LED_RED,    150); hal_delay_ms(150);
    led_on(LED_ALL);  hal_delay_ms(120);
    led_off(LED_ALL); hal_delay_ms(80);

    /* ========== 3. 初始化 LVGL ========== */
    lv_init();
    printf("[MAIN] LVGL initialized\n");

    /* ========== 4. 初始化显示和输入 ========== */
    lv_port_disp_init();
    lv_port_indev_init();

    /* ========== 5. 加载持久化数据 ========== */
    led_blink_n(LED_BLUE, 60, 2);
    tx_log_init();
    printf("[MAIN] Transaction log initialized (count=%d)\n", tx_log.count);

    led_blink_n(LED_BLUE, 60, 2);
    coupon_config_load();
    printf("[MAIN] Coupon config loaded\n");

    led_blink_n(LED_BLUE, 60, 2);
    price_config_load();
    printf("[MAIN] Price config loaded\n");

    /* ========== 6. 启动 UI ========== */
    shop_ui_init();
    printf("[MAIN] UI initialized, entering main loop\n");

    led_blink(LED_GREEN, 100); hal_delay_ms(80);
    led_blink(LED_GREEN, 100);

    /* ========== 7. 主循环 ========== */
    /* 注：LV_TICK_CUSTOM 已配置为 SDL_GetTicks()，无需手动调用 lv_tick_inc() */
    int running = 1;

    while (running) {
        /* 处理 SDL 事件 */
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            }
            lv_port_indev_handle_sdl_event(&event);
        }

        if (!running) break;

        /* LVGL 定时器处理和渲染 */
        lv_timer_handler();

        /* 更新显示（SDL_RenderPresent 会通过 vsync 限制帧率） */
        lv_port_disp_update();

        /* 微小延迟，防止 CPU 100% 占用 */
        SDL_Delay(1);
    }

    /* ========== 8. 清理 ========== */
    printf("[MAIN] Exiting...\n");
    lv_port_disp_deinit();
    SDL_Quit();
    return 0;
}
