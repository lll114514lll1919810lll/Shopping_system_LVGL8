/**
 * @file ff_stdio_adapter.c
 * @brief FATFS → stdio 适配层实现
 *
 * 将 "0:/xxx" 路径重定向到 "sim_data/xxx"。
 */

#include "ff.h"
#include <string.h>
#include <stdlib.h>

#define SIM_DATA_DIR "sim_data"

static const char * convert_path(const char *fatfs_path) {
    static char local_path[256];
    const char *p = fatfs_path;

    /* 跳过 "0:" 或 "1:" */
    if (p[0] && p[1] == ':') p += 2;
    /* 跳过开头的斜杠 */
    while (*p == '/' || *p == '\\') p++;

    snprintf(local_path, sizeof(local_path), SIM_DATA_DIR "/%s", p);
    return local_path;
}

FRESULT f_open(FIL *fp, const char *path, uint8_t mode) {
    if (!fp || !path) return FR_INVALID_PARAMETER;
    const char *local = convert_path(path);
    const char *mode_str;

    if (mode & FA_CREATE_ALWAYS) {
        mode_str = "wb+";
    } else if (mode & FA_CREATE_NEW) {
        mode_str = "wb";
    } else if (mode & FA_OPEN_ALWAYS) {
        /* 先尝试打开已有文件，不存在则创建 */
        fp->fp = fopen(local, "rb+");
        if (!fp->fp) fp->fp = fopen(local, "wb+");
        fp->mode = mode;
        return fp->fp ? FR_OK : FR_NO_FILE;
    } else if (mode & FA_OPEN_APPEND) {
        mode_str = "ab+";
    } else if (mode & FA_WRITE) {
        mode_str = "rb+";
    } else {
        mode_str = "rb";
    }

    fp->fp = fopen(local, mode_str);
    if (!fp->fp) return FR_NO_FILE;
    fp->mode = mode;
    return FR_OK;
}

FRESULT f_close(FIL *fp) {
    if (!fp || !fp->fp) return FR_INVALID_OBJECT;
    fclose(fp->fp);
    fp->fp = NULL;
    return FR_OK;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br) {
    if (!fp || !fp->fp) return FR_INVALID_OBJECT;
    size_t n = fread(buff, 1, btr, fp->fp);
    if (br) *br = (UINT)n;
    return (n == 0 && btr > 0) ? FR_DISK_ERR : FR_OK;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw) {
    if (!fp || !fp->fp) return FR_INVALID_OBJECT;
    size_t n = fwrite(buff, 1, btw, fp->fp);
    if (bw) *bw = (UINT)n;
    return (n < btw) ? FR_DISK_ERR : FR_OK;
}

FRESULT f_unlink(const char *path) {
    if (!path) return FR_INVALID_PARAMETER;
    return (remove(convert_path(path)) == 0) ? FR_OK : FR_NO_FILE;
}

FRESULT f_mount(void *fs, const char *path, uint8_t opt) {
    (void)fs; (void)path; (void)opt;
    return FR_OK;  /* PC 上无需挂载 */
}

FSIZE_t f_size(FIL *fp) {
    if (!fp || !fp->fp) return 0;
    long cur = ftell(fp->fp);
    fseek(fp->fp, 0, SEEK_END);
    long size = ftell(fp->fp);
    fseek(fp->fp, cur, SEEK_SET);
    return (FSIZE_t)size;
}

FRESULT f_stat(const char *path, FILINFO *fno) {
    if (!path || !fno) return FR_INVALID_PARAMETER;
    FILE *f = fopen(convert_path(path), "rb");
    if (!f) return FR_NO_FILE;
    fseek(f, 0, SEEK_END);
    fno->fsize = (uint32_t)ftell(f);
    fno->fdate = fno->ftime = 0;
    fclose(f);
    return FR_OK;
}
