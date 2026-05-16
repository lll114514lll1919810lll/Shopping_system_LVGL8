#ifndef _SHOP_APP_H
#define _SHOP_APP_H

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==================== 常量定义 ==================== */
#define MAX_PRODUCTS         5      // 商品种类数
#define MAX_CART_ITEMS       10     // 购物车最多商品项数
#define MAX_TX_HISTORY       20     // 历史交易记录最大条数
#define MAX_TX_ITEMS_PER_TX  10     // 单笔交易最多明细项数
#define TX_LOG_FILE          "0:/transactions.csv"

/* ==================== 数据结构定义 ==================== */

// 1. 商品结构体
typedef struct {
    uint8_t id;
    const char * name;
    uint32_t price;
    const char * unit;
    const char * img_path;
} product_t;

// 2. 交易明细项（单个商品在一笔交易中的记录）
typedef struct {
    uint8_t product_id;             // 商品ID（对应 shop_products 索引）
    float   quantity;               // 数量/重量
    float   subtotal;               // 小计金额（quantity * price）
} tx_item_t;

// 3. 优惠类型枚举（用于交易记录保存）
typedef enum {
    TX_DISC_NONE = 0,               // 无优惠
    TX_DISC_FULL_REDUCTION,         // 满减
    TX_DISC_PERCENT_OFF             // 折扣
} tx_discount_type_t;

// 4. 单笔交易记录
typedef struct {
    uint32_t          id;                        // 交易序号（自增）
    float             total_before_discount;      // 折扣前总价
    float             total_after_discount;       // 折扣后总价
    tx_discount_type_t discount_type;             // 使用的优惠类型
    uint8_t           item_count;                 // 明细项数
    tx_item_t         items[MAX_TX_ITEMS_PER_TX]; // 商品明细
} transaction_t;

// 5. 交易历史记录容器（RAM中的缓存）
typedef struct {
    transaction_t records[MAX_TX_HISTORY];  // 交易记录数组
    uint8_t       count;                     // 当前记录条数
    uint32_t      next_id;                   // 下一笔交易序号
} transaction_log_t;

/* ==================== 全局变量声明 ==================== */

extern product_t shop_products[MAX_PRODUCTS];
extern transaction_log_t tx_log;             // 交易历史记录
extern lv_obj_t * cart_list;
extern lv_obj_t * input_ta;
extern lv_obj_t * num_kb;
extern lv_obj_t * label_full;               // 覆盖全屏的背景label（用于隐藏键盘等）

/* ==================== 核心功能函数原型 ==================== */

void shop_ui_init(void);
void product_btn_event_cb(lv_event_t * e);
void kb_event_cb(lv_event_t * e);
void checkout_btn_event_cb(lv_event_t * e);
void clear_btn_event_cb(lv_event_t * e);
void label_event_cb(lv_event_t *e);
void discount_cb_event_cb(lv_event_t * e);
void create_discount_panel(lv_obj_t * parent);
void cart_list_btn_event_cb(lv_event_t* e);

/* ==================== 交易记录函数原型 ==================== */
void tx_log_init(void);                                     // 初始化交易记录
void tx_log_add(transaction_t * tx);                        // 添加一笔交易记录
void tx_log_clear(void);                                    // 清空所有交易记录
int  tx_log_save_to_sd(void);                               // 保存到 SD 卡
int  tx_log_load_from_sd(void);                             // 从 SD 卡加载

/* ==================== 底层接口 ==================== */
extern void * sdram_malloc(uint32_t size);
extern void read_file_to_array(const char *filename, void *array, uint32_t size);

#endif
