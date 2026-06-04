/**
 * @file drivers.h
 * @brief PC 模拟器硬件抽象层
 *
 * 替代原 Drivers/drivers.h，原项目 .c 文件通过 include path 优先级自动匹配此文件。
 */

#ifndef DRIVERS_H
#define DRIVERS_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "ff.h"
#include "lvgl.h"
#include "gd32h7xx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 图形 DMA 结构体桩 ==================== */
typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t *src;
    uint16_t *des;
} graphic_dma_struct;

/* ==================== 触摸点 ==================== */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t  size;
} lcd_touch_point_t;

extern lcd_touch_point_t tp[5];
extern graphic_dma_struct gdma;
extern __IO uint16_t gdma_lines;

/* ==================== LED 引脚 ==================== */
#define LED_GREEN   GPIO_PIN_0
#define LED_BLUE    GPIO_PIN_1
#define LED_YELLOW  GPIO_PIN_2
#define LED_RED     GPIO_PIN_3
#define LED_ALL     (LED_GREEN | LED_BLUE | LED_YELLOW | LED_RED)

/* ==================== LED 控制 ==================== */
/*
 * LED 宏已在 shop_app.h 中定义（使用 gpio_bit_* 和 delay_us）。
 * 此处不重复定义，让 shop_app.h 的宏自动适配我们的 gpio 桩和 delay_us 桩。
 * 为了更好的 PC 体验，提供 hal_delay_ms 供 main.c 使用。
 */
void hal_delay_ms(uint32_t ms);

/* ==================== GPIO 桩 ==================== */
void gpio_bit_reset(uint32_t gpio, uint32_t pin);
void gpio_bit_set(uint32_t gpio, uint32_t pin);
void gpio_bit_toggle(uint32_t gpio, uint32_t pin);
void rcu_periph_clock_enable(uint32_t periph);
void gpio_mode_set(uint32_t gpio, uint32_t mode, uint32_t pull, uint32_t pin);
void gpio_output_options_set(uint32_t gpio, uint32_t otype, uint32_t speed, uint32_t pin);

/* ==================== 系统函数 ==================== */
void systick_config(void);
void delay_us(uint32_t nus);
void sys_init(void);
void graphic_dma_copy(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t *src);

/* ==================== SDRAM 和文件读取 ==================== */
void *sdram_malloc(uint32_t size);
void read_file_to_array(const char *filename, void *array, uint32_t size);

/* ==================== 触摸屏桩 ==================== */
uint8_t atk_rgblcd_touch_scan(lcd_touch_point_t *points, uint8_t max_points);
void atk_rgblcd_touch_init(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_H */
