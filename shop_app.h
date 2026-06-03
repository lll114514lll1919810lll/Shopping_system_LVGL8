#ifndef _SHOP_APP_H
#define _SHOP_APP_H

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==================== 常量定义 ==================== */
#define MAX_PRODUCTS         6      // 商品种类数
#define MAX_CART_ITEMS       10     // 购物车最多商品项数
#define MAX_TX_HISTORY       30     // 历史交易记录最大条数
#define MAX_TX_ITEMS_PER_TX  10     // 单笔交易最多明细项数
#define TX_LOG_FILE          "0:/transactions.csv"
#define COUPON_CONFIG_FILE   "0:/coupon.dat"
#define PRICE_CONFIG_FILE    "0:/price.dat"

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
    TX_DISC_FULL_REDUCTION,         // 满20减5
    TX_DISC_PERCENT_OFF,            // 9折
    TX_DISC_FULL_100_80PCT,         // 满100打8折
    TX_DISC_FULL_200_50             // 满200减50
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

// 6. 键盘输入模式（决定确认后执行什么行为）
typedef enum {
    KB_MODE_NONE = 0,           // 空闲，无输入
    KB_MODE_ADD_TO_CART,        // 添加商品到购物车
    KB_MODE_EDIT_QUANTITY,      // 修改购物车中商品数量
} kb_input_mode_t;

/* ==================== 全局变量声明 ==================== */

extern product_t shop_products[MAX_PRODUCTS];
extern transaction_log_t tx_log;             // 交易历史记录
extern lv_obj_t * cart_list;
extern lv_obj_t * input_ta;
extern lv_obj_t * num_kb;
extern lv_obj_t * label_full;               // 覆盖全屏的背景label（用于隐藏键盘等）
extern int coupon_remaining[5];              // 每种优惠券剩余数量
extern lv_obj_t * discount_checkboxes[5];    // 优惠券复选框对象数组
extern int current_discount;                 // 当前选中的优惠类型

/* ==================== 字体声明 ==================== */
extern const lv_font_t ziti;                // 14px中文小字体
extern const lv_font_t ziti_title;          // 18px中文标题字体
extern const lv_font_t ziti_max;            // 24px中文大字体

/* ==================== 核心功能函数原型 ==================== */

void shop_ui_init(void);                    // 初始化UI（显示主页）
void show_home_screen(void);                // 显示主页
void show_shop_screen(void);                // 显示购物界面
void product_btn_event_cb(lv_event_t * e);
void kb_event_cb(lv_event_t * e);
void checkout_btn_event_cb(lv_event_t * e);
void clear_btn_event_cb(lv_event_t * e);
void label_event_cb(lv_event_t *e);
void discount_cb_event_cb(lv_event_t * e);
void create_discount_panel(lv_obj_t * parent);
void shop_ui_update_coupon_display(void);
void cart_list_btn_event_cb(lv_event_t* e);

/* 键盘输入界面控制（供其他模块复用键盘时调用） */
void show_input_ui(void);                    // 显示键盘输入界面
void hide_input_ui(void);                    // 隐藏键盘输入界面
extern kb_input_mode_t current_kb_mode;      // 当前键盘输入模式（设置后决定确认行为）

/* ==================== 交易记录函数原型 ==================== */
void tx_log_init(void);                                     // 初始化交易记录
void tx_log_add(transaction_t * tx);                        // 添加一笔交易记录
void tx_log_clear(void);                                    // 清空所有交易记录
int  tx_log_save_to_sd(void);                               // 保存到 SD 卡
int  tx_log_load_from_sd(void);                             // 从 SD 卡加载

/* ==================== 交易历史界面 ==================== */
void show_history_panel(void);                              // 显示交易历史面板
void history_btn_event_cb(lv_event_t * e);                  // 交易记录按钮回调
void shop_ui_close_shop_screen(void);                       // 关闭购物界面

/* ==================== 优惠券管理界面 ==================== */
void show_coupon_mgmt_screen(void);                         // 显示优惠券管理界面
void shop_ui_close_coupon_mgmt_screen(void);                // 关闭优惠券管理界面

/* ==================== 价格管理界面 ==================== */
void show_price_mgmt_screen(void);                              // 显示价格管理界面
void shop_ui_close_price_mgmt_screen(void);                     // 关闭价格管理界面

/* ==================== 价格配置持久化 ==================== */
void price_config_load(void);                              // 从SD卡加载商品价格
void price_config_save(void);                              // 保存商品价格到SD卡

/* ==================== 优惠券配置持久化 ==================== */
void coupon_config_load(void);                              // 从SD卡加载优惠券数量
void coupon_config_save(void);                              // 保存优惠券数量到SD卡

/* ==================== 底层接口 ==================== */
extern void * sdram_malloc(uint32_t size);
extern void read_file_to_array(const char *filename, void *array, uint32_t size);

/* ==================== UI 辅助函数 ==================== */
lv_obj_t * shop_ui_show_msgbox(const char * title, const char * message, const lv_color_t * txt_color);
void shop_ui_add_cart_item(const char * item_text, const char * product_name);
void shop_ui_show_checkout_result(const transaction_t * tx, float grand_total, float final_total, const char * discount_desc);
lv_obj_t * shop_ui_show_cart_action_menu(void);
lv_obj_t * shop_ui_get_cart_delete_btn(void);              // 获取购物车操作菜单的删除按钮
lv_obj_t * shop_ui_get_cart_edit_btn(void);                // 获取购物车操作菜单的修改按钮
void shop_ui_close_cart_menu(void);                        // 关闭购物车操作菜单
void shop_ui_close_history_panel(void);                          // 关闭交易历史面板
void shop_ui_refresh_history_list(void);                         // 刷新交易历史列表
void shop_ui_show_tx_detail(transaction_t * tx);                 // 显示交易明细
void shop_ui_update_cart_total(void);                            // 更新购物车总价显示

#endif
