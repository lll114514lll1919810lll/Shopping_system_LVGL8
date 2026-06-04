/**
 * @file lv_port_indev.h
 * @brief SDL2 输入驱动头文件
 */

#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#include "lvgl.h"
#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

void lv_port_indev_init(void);

/**
 * 处理 SDL 事件中的鼠标/触摸输入
 * 应在主循环中每次 SDL_PollEvent 后调用
 * @return 1 表示发生了 LVGL 交互事件，0 表示无交互
 */
int lv_port_indev_handle_sdl_event(const SDL_Event *event);

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_INDEV_H */
