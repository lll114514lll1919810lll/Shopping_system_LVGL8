/**
 * @file lv_port_disp.h
 * @brief SDL2 显示驱动头文件
 */

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_port_disp_init(void);
void lv_port_disp_deinit(void);
void lv_port_disp_update(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_DISP_H */
