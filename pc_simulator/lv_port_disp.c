/**
 * @file lv_port_disp.c
 * @brief SDL2 显示驱动实现
 *
 * 使用全帧缓冲方案：LVGL 渲染到完整的 1024×600 缓冲中，
 * 每帧结束后整帧上传到 SDL 纹理并呈现。
 */

#include "lv_port_disp.h"
#include "drivers.h"
#include <SDL.h>

#define DISP_HOR_RES  1024
#define DISP_VER_RES  600

static SDL_Window   *sdl_window   = NULL;
static SDL_Renderer *sdl_renderer = NULL;
static SDL_Texture  *sdl_texture  = NULL;

/* 全帧缓冲（约 1.2MB，PC 上完全可接受） */
static lv_color_t *full_fb = NULL;

/* ==================== LVGL flush 回调 ==================== */
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    /*
     * LVGL 已完成对 area 区域的渲染，数据在 color_p。
     * 因为是全帧缓冲，color_p == full_fb + offset，
     * 且已经是最终位置。无需拷贝，只需告知 LVGL 刷新完成。
     *
     * 我们将 SDL 纹理更新推迟到主循环中进行（一次性上传整帧）。
     */
    (void)area;
    (void)color_p;
    lv_disp_flush_ready(disp_drv);
}

/* ==================== 初始化 ==================== */
void lv_port_disp_init(void) {
    /* 创建 SDL 窗口 */
    sdl_window = SDL_CreateWindow(
        "Shopping System - LVGL PC Simulator (1024x600)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        DISP_HOR_RES, DISP_VER_RES,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!sdl_window) {
        printf("[DISP] ERROR: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return;
    }

    /* 创建渲染器 */
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdl_renderer) {
        printf("[DISP] ERROR: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return;
    }

    /* 创建流式纹理（RGB565 格式，与 LVGL 色深完全匹配） */
    sdl_texture = SDL_CreateTexture(sdl_renderer,
        SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
        DISP_HOR_RES, DISP_VER_RES);
    if (!sdl_texture) {
        printf("[DISP] ERROR: SDL_CreateTexture failed: %s\n", SDL_GetError());
        return;
    }

    /* 分配全帧缓冲 */
    full_fb = (lv_color_t *)malloc(DISP_HOR_RES * DISP_VER_RES * sizeof(lv_color_t));
    if (!full_fb) {
        printf("[DISP] ERROR: Failed to allocate full frame buffer\n");
        return;
    }

    /* LVGL draw buffer: 一个全屏大小 + 一个小的第二缓冲 */
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, full_fb, NULL, DISP_HOR_RES * DISP_VER_RES);

    /* 注册显示驱动 */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = DISP_HOR_RES;
    disp_drv.ver_res  = DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    /* 使用全帧缓冲则启用 direct_mode，LVGL 直接渲染到 full_fb */
    disp_drv.direct_mode = 1;
    lv_disp_drv_register(&disp_drv);

    printf("[DISP] Initialized: %dx%d RGB565, direct mode\n", DISP_HOR_RES, DISP_VER_RES);
}

void lv_port_disp_deinit(void) {
    if (full_fb)      { free(full_fb); full_fb = NULL; }
    if (sdl_texture)  { SDL_DestroyTexture(sdl_texture); sdl_texture = NULL; }
    if (sdl_renderer) { SDL_DestroyRenderer(sdl_renderer); sdl_renderer = NULL; }
    if (sdl_window)   { SDL_DestroyWindow(sdl_window); sdl_window = NULL; }
}

/* ==================== 帧更新 ==================== */
void lv_port_disp_update(void) {
    if (!sdl_renderer || !sdl_texture || !full_fb) return;

    /* 上传整帧到 SDL 纹理 */
    void *pixels;
    int pitch;
    if (SDL_LockTexture(sdl_texture, NULL, &pixels, &pitch) != 0) {
        return;
    }
    memcpy(pixels, full_fb, DISP_HOR_RES * DISP_VER_RES * sizeof(lv_color_t));
    SDL_UnlockTexture(sdl_texture);

    /* 渲染 */
    SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);
}
