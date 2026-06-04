/**
 * @file hal_stubs.c
 * @brief 硬件抽象层桩实现
 */

#include "drivers.h"
#include <windows.h>
#include <stdlib.h>

/* ==================== 全局变量 ==================== */
lcd_touch_point_t tp[5] = {0};
graphic_dma_struct gdma;
__IO uint16_t gdma_lines;

/* ==================== LED 辅助 ==================== */

static const char * led_name(uint32_t pin) {
    switch (pin) {
        case GPIO_PIN_0: return "GREEN";
        case GPIO_PIN_1: return "BLUE";
        case GPIO_PIN_2: return "YELLOW";
        case GPIO_PIN_3: return "RED";
        default: return "?";
    }
}

/* ==================== GPIO 桩（含 LED 输出） ==================== */

void gpio_bit_reset(uint32_t gpio, uint32_t pin) {
    (void)gpio;
    if (pin == LED_ALL)
        printf("[LED] ALL ON\n");
    else if (pin <= GPIO_PIN_3)
        printf("[LED] %s ON\n", led_name(pin));
}

void gpio_bit_set(uint32_t gpio, uint32_t pin) {
    (void)gpio;
    /* LED OFF 静默，减少控制台噪音 */
}

void gpio_bit_toggle(uint32_t gpio, uint32_t pin) {
    (void)gpio;
    printf("[LED] %s TOGGLE\n", led_name(pin));
}

void rcu_periph_clock_enable(uint32_t periph)         { (void)periph; }
void gpio_mode_set(uint32_t g, uint32_t m, uint32_t p, uint32_t pin) { (void)g;(void)m;(void)p;(void)pin; }
void gpio_output_options_set(uint32_t g, uint32_t o, uint32_t s, uint32_t pin) { (void)g;(void)o;(void)s;(void)pin; }

/* ==================== 延时 ==================== */

void hal_delay_ms(uint32_t ms) {
    Sleep(ms);
}

void delay_us(uint32_t nus) {
    /* Windows Sleep 精度有限，>1ms 时用 Sleep，<1ms 忙等 */
    if (nus > 1000) {
        Sleep(nus / 1000);
    }
}

/* ==================== 系统 ==================== */

void systick_config(void) { }

void sys_init(void) {
    printf("[SIM] Hardware init skipped (PC simulator)\n");
}

/* ==================== 图形 DMA ==================== */

void graphic_dma_copy(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t *src) {
    (void)x1; (void)x2; (void)y1; (void)y2; (void)src;
    gdma_lines = 0;
}

/* ==================== SDRAM 内存 ==================== */

void *sdram_malloc(uint32_t size) {
    return malloc(size);
}

/* ==================== 文件读取（签名匹配 shop_app.h） ==================== */

void read_file_to_array(const char *filename, void *array, uint32_t max_size) {
    FIL file;
    UINT bytes_read;

    if (f_open(&file, filename, FA_READ) != FR_OK) {
        f_mount(NULL, "", 0);
        return;
    }

    FSIZE_t file_size = f_size(&file);
    if (file_size > max_size) {
        f_close(&file);
        f_mount(NULL, "", 0);
        return;
    }

    f_read(&file, array, (UINT)file_size, &bytes_read);
    f_close(&file);
}

/* ==================== 触摸屏桩 ==================== */

void atk_rgblcd_touch_init(void) { }

uint8_t atk_rgblcd_touch_scan(lcd_touch_point_t *points, uint8_t max_points) {
    (void)points; (void)max_points;
    /* 触摸坐标由 lv_port_indev.c 通过 SDL 事件直接写入 tp[] */
    if (tp[0].x != 0 || tp[0].y != 0) {
        return 1;
    }
    return 0;
}
