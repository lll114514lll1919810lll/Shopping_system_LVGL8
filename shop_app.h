#ifndef _SHOP_APP_H
#define _SHOP_APP_H

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 1. 商品结构体定义
typedef struct {
    uint8_t id;
    const char * name;
    uint32_t price;
    const char * unit;
    const char * img_path;
} product_t;

// 2. 全局变量声明
extern product_t shop_products[5];
extern lv_obj_t * cart_list;
extern lv_obj_t * input_ta;
extern lv_obj_t * num_kb;

// 3. 核心功能函数原型
void shop_ui_init(void);
void product_btn_event_cb(lv_event_t * e);
void kb_event_cb(lv_event_t * e);
void checkout_btn_event_cb(lv_event_t * e);
void clear_btn_event_cb(lv_event_t * e);
void label_event_cb(lv_event_t *e);

// 4. 底层接口（由 main 或相关底层文件提供）
extern void * sdram_malloc(uint32_t size);
extern void read_file_to_array(const char *filename, void *array, uint32_t size);

#endif
