/**
 * @file ff.h
 * @brief PC 模拟器 FATFS → stdio 适配头文件
 *
 * 完整模拟 FatFs API，将所有文件操作映射到标准 C 文件 I/O。
 * 自动去除路径中的 "0:" 前缀，重定向到本地 sim_data/ 目录。
 */

#ifndef FF_H
#define FF_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 类型定义 ==================== */

typedef struct {
    FILE *fp;
    uint8_t mode;
} FIL;

typedef struct {
    int dummy;
} DIR;

typedef struct {
    uint32_t fsize;
    uint16_t fdate;
    uint16_t ftime;
    uint8_t  fattrib;          /* 文件属性标志，用于 lv_fs_fatfs */
    char     fname[256];       /* 短文件名，用于 lv_fs_fatfs */
} FILINFO;

typedef enum {
    FR_OK = 0,
    FR_DISK_ERR,
    FR_INT_ERR,
    FR_NOT_READY,
    FR_NO_FILE,
    FR_NO_PATH,
    FR_INVALID_NAME,
    FR_DENIED,
    FR_EXIST,
    FR_INVALID_OBJECT,
    FR_WRITE_PROTECTED,
    FR_INVALID_DRIVE,
    FR_NOT_ENABLED,
    FR_NO_FILESYSTEM,
    FR_MKFS_ABORTED,
    FR_TIMEOUT,
    FR_LOCKED,
    FR_NOT_ENOUGH_CORE,
    FR_TOO_MANY_OPEN_FILES,
    FR_INVALID_PARAMETER
} FRESULT;

typedef uint32_t FSIZE_t;
typedef unsigned int UINT;

/* 文件访问模式 */
#define FA_READ          0x01
#define FA_WRITE         0x02
#define FA_OPEN_EXISTING 0x00
#define FA_CREATE_NEW    0x04
#define FA_CREATE_ALWAYS 0x08
#define FA_OPEN_ALWAYS   0x10
#define FA_OPEN_APPEND   0x30

#define AM_RDO  0x01
#define AM_HID  0x02
#define AM_SYS  0x04
#define AM_DIR  0x10
#define AM_ARC  0x20

/* ==================== API 声明 ==================== */

FRESULT f_open(FIL *fp, const char *path, uint8_t mode);
FRESULT f_close(FIL *fp);
FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br);
FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw);
FRESULT f_unlink(const char *path);
FRESULT f_mount(void *fs, const char *path, uint8_t opt);
FSIZE_t f_size(FIL *fp);
FRESULT f_stat(const char *path, FILINFO *fno);

#ifdef __cplusplus
}
#endif

#endif /* FF_H */
