cmake_minimum_required(VERSION 3.15)
project(shopping_sim C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================
# SDL2
# ============================================================
find_package(SDL2 REQUIRED)

# ============================================================
# 路径
# ============================================================
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# ============================================================
# 源文件
# ============================================================

# 1. PC 模拟器源文件
set(SIM_SOURCES
    main.c
    lv_port_disp.c
    lv_port_indev.c
    hal_stubs.c
    ff_stdio_adapter.c
)

# 2. 原项目业务逻辑源文件（不修改，直接编译）
set(APP_SOURCES
    ${PROJECT_ROOT}/shop_ui.c
    ${PROJECT_ROOT}/shop_callbacks.c
    ${PROJECT_ROOT}/tx_log.c
)

# 3. 自定义中文字体
set(FONT_SOURCES
    ${PROJECT_ROOT}/lvgl/user_lv_font/ziti.c
    ${PROJECT_ROOT}/lvgl/user_lv_font/ziti_title.c
    ${PROJECT_ROOT}/lvgl/user_lv_font/ziti_max.c
)

# 4. LVGL 库源文件（自动收集全部）
file(GLOB_RECURSE LVGL_SOURCES
    ${PROJECT_ROOT}/lvgl/src/*.c
)
# 排除模板文件
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*lv_objx_templ\\.c$")
# 排除示例 porting 文件（我们用自定义的）
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*porting.*")

# 5. LVGL 内置字体（Montserrat 14/18/24，已启用）
# 已通过 file(GLOB_RECURSE ...) 自动收集

# ============================================================
# 可执行文件
# ============================================================
add_executable(${PROJECT_NAME}
    ${SIM_SOURCES}
    ${APP_SOURCES}
    ${FONT_SOURCES}
    ${LVGL_SOURCES}
)

# ============================================================
# 头文件搜索路径
# ============================================================
# 关键：pc_simulator 目录在最前面，使其中的 ff.h、drivers.h、
# gd32h7xx.h、lv_conf.h 优先于原始版本
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}           # pc_simulator/ (最高优先级)
    ${PROJECT_ROOT}                       # 根目录 (shop_app.h, shop_chinese.h)
    ${PROJECT_ROOT}/lvgl                  # lvgl.h
    ${PROJECT_ROOT}/lvgl/src              # LVGL 内部引用
    ${SDL2_INCLUDE_DIRS}                  # SDL2 头文件
)

# ============================================================
# 宏定义
# ============================================================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    LV_CONF_INCLUDE_SIMPLE                # 令 LVGL 在 include path 中搜索 lv_conf.h
    PC_SIMULATOR                          # 禁用 shop_ui.c 中硬件相关的 sdram_malloc
)

# ============================================================
# 链接
# ============================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
)

# ============================================================
# MSVC 编译选项
# ============================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /utf-8
        /W3
        /wd4100
        /wd4204
    )
endif()
