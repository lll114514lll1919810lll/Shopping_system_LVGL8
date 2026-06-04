/**
 * @file lv_port_indev.c
 * @brief SDL2 鼠标/触摸输入驱动实现
 *
 * 将 SDL 鼠标事件映射为 LVGL 触摸输入设备。
 * 支持左键按下 = 触摸按下，移动 = 拖拽，左键释放 = 触摸释放。
 * 同时将 SDL 键盘事件转发给 LVGL 键盘输入设备。
 */

#include "lv_port_indev.h"
#include "drivers.h"
#include <SDL.h>

/* ==================== 鼠标/触摸状态 ==================== */
static lv_coord_t mouse_x = 0;
static lv_coord_t mouse_y = 0;
static int mouse_pressed = 0;

/* 输入的 LVGL 设备 */
static lv_indev_t *indev_pointer = NULL;
static lv_group_t *input_group = NULL;

/* ==================== 触摸回调 ==================== */
static void touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    (void)indev;

    /* 将鼠标坐标传递给 LVGL */
    data->point.x = mouse_x;
    data->point.y = mouse_y;
    data->state   = mouse_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    /* 同时更新 tp[]（供原项目中的触摸扫描函数使用） */
    tp[0].x = (uint16_t)mouse_x;
    tp[0].y = (uint16_t)mouse_y;
}

/* ==================== 键盘回调（可选 — 支持硬件按键导航） ==================== */
static uint32_t last_key = 0;

static void keyboard_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    (void)indev;
    data->key   = last_key;
    data->state = last_key ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    last_key = 0;  /* 只发送一次 */
}

/* ==================== 初始化 ==================== */
void lv_port_indev_init(void) {
    /* 触摸/鼠标输入设备 */
    static lv_indev_drv_t indev_drv_pointer;
    lv_indev_drv_init(&indev_drv_pointer);
    indev_drv_pointer.type    = LV_INDEV_TYPE_POINTER;
    indev_drv_pointer.read_cb = touchpad_read;
    indev_pointer = lv_indev_drv_register(&indev_drv_pointer);

    /* 键盘输入设备（用于数字键盘导航） */
    static lv_indev_drv_t indev_drv_kb;
    lv_indev_drv_init(&indev_drv_kb);
    indev_drv_kb.type    = LV_INDEV_TYPE_KEYPAD;
    indev_drv_kb.read_cb = keyboard_read;
    lv_indev_drv_register(&indev_drv_kb);

    printf("[INDEV] Mouse + Keyboard input initialized\n");
}

/* ==================== SDL 事件处理 ==================== */
int lv_port_indev_handle_sdl_event(const SDL_Event *event) {
    switch (event->type) {

    case SDL_MOUSEBUTTONDOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            mouse_x = (lv_coord_t)event->button.x;
            mouse_y = (lv_coord_t)event->button.y;
            mouse_pressed = 1;
            return 1;
        }
        break;

    case SDL_MOUSEBUTTONUP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            mouse_x = (lv_coord_t)event->button.x;
            mouse_y = (lv_coord_t)event->button.y;
            mouse_pressed = 0;
            return 1;
        }
        break;

    case SDL_MOUSEMOTION:
        if (mouse_pressed) {
            mouse_x = (lv_coord_t)event->motion.x;
            mouse_y = (lv_coord_t)event->motion.y;
            return 1;
        }
        break;

    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        case SDLK_ESCAPE:
            last_key = LV_KEY_ESC;
            return 1;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            last_key = LV_KEY_ENTER;
            return 1;
        case SDLK_BACKSPACE:
            last_key = LV_KEY_BACKSPACE;
            return 1;
        case SDLK_LEFT:
            last_key = LV_KEY_LEFT;
            return 1;
        case SDLK_RIGHT:
            last_key = LV_KEY_RIGHT;
            return 1;
        case SDLK_UP:
            last_key = LV_KEY_UP;
            return 1;
        case SDLK_DOWN:
            last_key = LV_KEY_DOWN;
            return 1;
        default:
            /* 将数字和字母发送给 LVGL 键盘 */
            if (event->key.keysym.sym >= SDLK_0 && event->key.keysym.sym <= SDLK_9) {
                last_key = event->key.keysym.sym;
                return 1;
            }
            break;
        }
        break;

    case SDL_WINDOWEVENT:
        /* 窗口大小改变时可以在此处理缩放 */
        break;

    default:
        break;
    }

    return 0;
}
