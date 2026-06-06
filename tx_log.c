#include "shop_app.h"
#include "shop_chinese.h"
#include "ff.h"
#include "drivers.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// 全局交易记录
transaction_log_t tx_log;

// 初始化
void tx_log_init(void)
{
    tx_log.count   = 0;
    tx_log.next_id = 1;
    tx_log_load_from_sd();
}

// 添加一条交易记录
void tx_log_add(transaction_t * tx)
{
    if(tx == NULL) return;

    // 满了就挤掉最旧那条
    if(tx_log.count >= MAX_TX_HISTORY) {
        for(uint8_t i = 0; i < tx_log.count - 1; i++) {
            tx_log.records[i] = tx_log.records[i + 1];
        }
        tx_log.count--;
    }

    tx->id = tx_log.next_id++;
    tx_log.records[tx_log.count] = *tx;
    tx_log.count++;
    tx_log_save_to_sd();
}

// 清空全部记录
void tx_log_clear(void)
{
    tx_log.count   = 0;
    tx_log.next_id = 1;
    f_unlink(TX_LOG_FILE);
}

// 删除指定索引的记录
void tx_log_delete(uint8_t index)
{
    if(index >= tx_log.count) return;

    for(uint8_t i = index; i < tx_log.count - 1; i++) {
        tx_log.records[i] = tx_log.records[i + 1];
    }
    tx_log.count--;

    // 重新编号，后面的序号补位
    for(uint8_t i = 0; i < tx_log.count; i++) {
        tx_log.records[i].id = i + 1;
    }
    tx_log.next_id = tx_log.count + 1;

    tx_log_save_to_sd();
}

// 保存到 SD 卡，CSV 格式
int tx_log_save_to_sd(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_written;
    led_blink_n(LED_BLUE, 60, 2);

    res = f_open(&file, TX_LOG_FILE, FA_WRITE | FA_CREATE_ALWAYS);
    if(res != FR_OK) return -1;

    // 写表头
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

    for(uint8_t i = 0; i < tx_log.count; i++) {
        transaction_t * tx = &tx_log.records[i];
        int offset = 0;

        offset += snprintf(line_buf + offset, sizeof(line_buf) - offset,
                           "%lu,%.2f,%.2f,%d,%d",
                           (unsigned long)tx->id,
                           tx->total_before_discount,
                           tx->total_after_discount,
                           (int)tx->discount_type,
                           (int)tx->item_count);

        // 固定写10个条目位置，空的填0
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

// 从 SD 卡加载记录
int tx_log_load_from_sd(void)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    char file_buf[8192];
    led_blink_n(LED_BLUE, 60, 2);

    res = f_open(&file, TX_LOG_FILE, FA_READ);
    if(res != FR_OK) {
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

    // 跳过第一行（表头）
    char * p = file_buf;
    while(*p && *p != '\n') p++;
    if(*p == '\n') p++;

    tx_log.count   = 0;
    tx_log.next_id = 1;

    // 逐行解析
    while(*p && tx_log.count < MAX_TX_HISTORY) {
        transaction_t * tx = &tx_log.records[tx_log.count];
        memset(tx, 0, sizeof(transaction_t));

        // 跳过空格
        while(*p == ' ' || *p == '\t') p++;
        if(*p == '\0' || *p == '\r' || *p == '\n') {
            while(*p && *p != '\n') p++;
            if(*p == '\n') p++;
            continue;
        }

        // 读基本字段
        tx->id = (uint32_t)strtol(p, &p, 10); if(*p == ',') p++;
        tx->total_before_discount = (float)strtod(p, &p); if(*p == ',') p++;
        tx->total_after_discount  = (float)strtod(p, &p); if(*p == ',') p++;
        tx->discount_type = (tx_discount_type_t)strtol(p, &p, 10); if(*p == ',') p++;
        tx->item_count = (uint8_t)strtol(p, &p, 10);

        // 读商品明细
        for(uint8_t j = 0; j < tx->item_count && j < MAX_TX_ITEMS_PER_TX; j++) {
            if(*p == ',') p++;
            tx->items[j].product_id = (uint8_t)strtol(p, &p, 10); if(*p == ',') p++;
            tx->items[j].quantity   = (float)strtod(p, &p);       if(*p == ',') p++;
            tx->items[j].subtotal   = (float)strtod(p, &p);
        }

        // 跳过剩余空位
        for(uint8_t j = tx->item_count; j < MAX_TX_ITEMS_PER_TX; j++) {
            if(*p == ',') p++;
            strtol(p, &p, 10);
            if(*p == ',') p++;
            strtod(p, &p);
            if(*p == ',') p++;
            strtod(p, &p);
        }

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
