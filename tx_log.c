#include "shop_app.h"
#include "shop_chinese.h"
#include "ff.h"
#include "drivers.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * tx_log.c - 交易记录管理
 *
 * 功能：
 *   1. 在 RAM 中维护交易历史缓存
 *   2. 结算时将交易记录追加到 SD 卡的 CSV 文件
 *   3. 支持加载、清空历史记录
 *
 * CSV 格式:
 *   ID, 折扣前总价, 折扣后总价, 优惠类型, 商品数,
 *   商品1_ID,商品1_数量,商品1_小计,
 *   商品2_ID,商品2_数量,商品2_小计,
 *   ...
 */

// 全局交易历史记录缓存
transaction_log_t tx_log;

/* 初始化交易记录（RAM缓存 + 从SD卡加载） */
void tx_log_init(void)
{
    tx_log.count   = 0;
    tx_log.next_id = 1;
    tx_log_load_from_sd();
}

/* 记录一笔交易到 RAM 缓存 */
void tx_log_add(transaction_t * tx)
{
    if(tx == NULL) return;

    // 如果缓存已满，移除最早的一条（队列移位）
    if(tx_log.count >= MAX_TX_HISTORY) {
        for(uint8_t i = 0; i < tx_log.count - 1; i++) {
            tx_log.records[i] = tx_log.records[i + 1];
        }
        tx_log.count--;
    }

    // 分配交易序号
    tx->id = tx_log.next_id++;
    tx_log.records[tx_log.count] = *tx;
    tx_log.count++;

    // 自动保存到 SD 卡
    tx_log_save_to_sd();
}

/* 清空所有交易记录 */
void tx_log_clear(void)
{
    tx_log.count   = 0;
    tx_log.next_id = 1;

    // 删除 SD 卡上的文件
    f_unlink(TX_LOG_FILE);
}

/* 将交易记录保存到 SD 卡 (追加模式) */
int tx_log_save_to_sd(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_written;
    led_blink_n(LED_BLUE, 80, 2);

    // 每次都重写整个文件（保证文件内容与 RAM 缓存一致）
    res = f_open(&file, TX_LOG_FILE, FA_WRITE | FA_CREATE_ALWAYS);
    if(res != FR_OK) return -1;

    // 写 CSV 表头
    const char * header = "ID,DiscountBefore,DiscountAfter,DiscountType,ItemCount,"
                          "Item1_ID,Item1_Qty,Item1_Sub,"
                          "Item2_ID,Item2_Qty,Item2_Sub,"
                          "Item3_ID,Item3_Qty,Item3_Sub,"
                          "Item4_ID,Item4_Qty,Item4_Sub,"
                          "Item5_ID,Item5_Qty,Item5_Sub,"
                          "Item6_ID,Item6_Qty,Item6_Sub,"
                          "Item7_ID,Item7_Qty,Item7_Sub,"
                          "Item8_ID,Item8_Qty,Item8_Sub,"
                          "Item9_ID,Item9_Qty,Item9_Sub,"
                          "Item10_ID,Item10_Qty,Item10_Sub\r\n";
    f_write(&file, header, strlen(header), &bytes_written);

    char line_buf[256];

    // 写每条交易记录
    for(uint8_t i = 0; i < tx_log.count; i++) {
        transaction_t * tx = &tx_log.records[i];
        int offset = 0;

        // 基本字段
        offset += snprintf(line_buf + offset, sizeof(line_buf) - offset,
                           "%lu,%.2f,%.2f,%d,%d",
                           (unsigned long)tx->id,
                           tx->total_before_discount,
                           tx->total_after_discount,
                           (int)tx->discount_type,
                           (int)tx->item_count);

        // 明细项（固定10个字段，不足的补 0,0,0）
        for(uint8_t j = 0; j < MAX_TX_ITEMS_PER_TX; j++) {
            if(j < tx->item_count) {
                offset += snprintf(line_buf + offset, sizeof(line_buf) - offset,
                                   ",%d,%.1f,%.2f",
                                   (int)tx->items[j].product_id,
                                   tx->items[j].quantity,
                                   tx->items[j].subtotal);
            } else {
                offset += snprintf(line_buf + offset, sizeof(line_buf) - offset,
                                   ",0,0.0,0.00");
            }
        }

        offset += snprintf(line_buf + offset, sizeof(line_buf) - offset, "\r\n");
        f_write(&file, line_buf, strlen(line_buf), &bytes_written);
    }

    f_close(&file);
    return 0;
}

/* 从 SD 卡加载交易记录到 RAM 缓存 */
int tx_log_load_from_sd(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    char file_buf[8192]; // 足够容纳 20 条记录
    led_blink_n(LED_BLUE, 80, 2);

    res = f_open(&file, TX_LOG_FILE, FA_READ);
    if(res != FR_OK) {
        // 文件不存在是正常的（首次使用）
        return 0;
    }

    FSIZE_t file_size = f_size(&file);
    if(file_size >= sizeof(file_buf)) {
        f_close(&file);
        return -1;
    }

    res = f_read(&file, file_buf, (UINT)file_size, &bytes_read);
    f_close(&file);
    if(res != FR_OK) return -1;

    // 跳过 CSV 表头（找到第一个换行符后的第一个字符）
    char * p = file_buf;
    while(*p && *p != '\n') p++;
    if(*p == '\n') p++;

    tx_log.count   = 0;
    tx_log.next_id = 1;

    // 逐行解析
    while(*p && tx_log.count < MAX_TX_HISTORY) {
        transaction_t * tx = &tx_log.records[tx_log.count];
        memset(tx, 0, sizeof(transaction_t));

        // 跳过行首空白
        while(*p == ' ' || *p == '\t') p++;
        if(*p == '\0' || *p == '\r' || *p == '\n') {
            // 空行，跳到下一行
            while(*p && *p != '\n') p++;
            if(*p == '\n') p++;
            continue;
        }

        // 解析: ID,DiscountBefore,DiscountAfter,DiscountType,ItemCount
        tx->id = (uint32_t)strtol(p, &p, 10); if(*p == ',') p++;
        tx->total_before_discount = (float)strtod(p, &p); if(*p == ',') p++;
        tx->total_after_discount  = (float)strtod(p, &p); if(*p == ',') p++;
        tx->discount_type = (tx_discount_type_t)strtol(p, &p, 10); if(*p == ',') p++;
        tx->item_count = (uint8_t)strtol(p, &p, 10);

        // 解析明细项
        for(uint8_t j = 0; j < tx->item_count && j < MAX_TX_ITEMS_PER_TX; j++) {
            if(*p == ',') p++;
            tx->items[j].product_id = (uint8_t)strtol(p, &p, 10); if(*p == ',') p++;
            tx->items[j].quantity   = (float)strtod(p, &p);       if(*p == ',') p++;
            tx->items[j].subtotal   = (float)strtod(p, &p);
        }

        // 跳过剩余未解析的字段（0填充的空位）
        for(uint8_t j = tx->item_count; j < MAX_TX_ITEMS_PER_TX; j++) {
            if(*p == ',') p++;
            strtol(p, &p, 10);  // skip ID
            if(*p == ',') p++;
            strtod(p, &p);      // skip Qty
            if(*p == ',') p++;
            strtod(p, &p);      // skip Sub
        }

        // 更新 next_id
        if(tx->id >= tx_log.next_id) {
            tx_log.next_id = tx->id + 1;
        }
        tx_log.count++;

        // 跳到下一行
        while(*p && *p != '\n') p++;
        if(*p == '\n') p++;
    }

    return 0;
}
