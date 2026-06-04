/**
 * @file gd32h7xx.h
 * @brief PC 模拟器用的 gd32h7xx 头文件桩
 *
 * 原项目 shop_ui.c 直接 #include "gd32h7xx.h"，
 * 此桩文件提供所需的基本类型定义，避免修改原始代码。
 */

#ifndef GD32H7XX_H
#define GD32H7XX_H

#include <stdint.h>
#include <stdbool.h>

/* GD32 外设基地址桩（仅用于类型兼容，不实际访问硬件） */
#define __IO  volatile
#define __IM  volatile const
#define __OM  volatile

/* GPIO 相关桩 — 用 uint32_t 类型避免与 shop_app.h 中的宏产生指针/整数转换警告 */
typedef uint32_t GPIO_TypeDef;
#define GPIOA ((uint32_t)0)

#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_2  ((uint16_t)0x0004)
#define GPIO_PIN_3  ((uint16_t)0x0008)

#define GPIO_MODE_OUTPUT    ((uint8_t)0x01)
#define GPIO_PUPD_NONE      ((uint8_t)0x00)
#define GPIO_OTYPE_PP       ((uint8_t)0x01)
#define GPIO_OSPEED_60MHZ   ((uint8_t)0x03)

/* RCU 外设桩 */
#define RCU_GPIOA   ((uint32_t)0)

/* PC 模拟器下自动引入硬件桩声明，使 shop_ui.c 等
   文件中的 gpio_bit_* / delay_us / led_* 宏可用 */
#ifdef PC_SIMULATOR
#include "drivers.h"
#endif

#endif /* GD32H7XX_H */
