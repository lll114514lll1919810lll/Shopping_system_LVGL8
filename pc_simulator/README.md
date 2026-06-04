# PC 模拟器

在 Windows 上模拟运行购物系统 LVGL UI，无需开发板硬件。

## 依赖

- **Visual Studio 2022**（或 2019）含 C++ 桌面开发工作负载
- **CMake** ≥ 3.15
- **SDL2** 开发库

## 快速开始

### 1. 安装 SDL2

```powershell
# 方式 A：用 vcpkg（推荐）
vcpkg install sdl2:x64-windows

# 方式 B：手动下载
# 从 https://github.com/libsdl-org/SDL/releases 下载 SDL2-devel-2.x.x-VC.zip
# 解压到 C:\SDL2
```

### 2. 准备模拟数据

将开发板 SD 卡中的文件复制到 `pc_simulator/sim_data/` 目录下：

```
sim_data/
├── apple.bin          # 200×200 RGB565 商品图片
├── milk.bin
├── bread.bin
├── watermelon.bin
├── cola.bin
├── chocolate.bin
├── background.bin     # 1024×600 RGB565 背景图
├── transactions.csv   # 交易记录（可空文件）
├── coupon.dat         # 优惠券配置（可空文件）
├── price.dat          # 价格配置（可空文件）
```

如果没有图片，模拟器仍可运行，只是商品图片区域显示空白。

### 3. 编译运行

```powershell
cd pc_simulator
mkdir build
cd build

# 如果 SDL2 安装在 C:\SDL2：
cmake .. -DSDL2_DIR=C:/SDL2/cmake

# 如果用 vcpkg：
cmake .. -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake

cmake --build . --config Release
.\Release\shopping_sim.exe
```

> 注：不确定编译产物的具体目录，请根据实际情况处理。

### 4. 操作方式

- **鼠标点击** = 触摸屏操作
- **键盘输入** = LVGL 键盘输入（数字键盘等）
- 关闭窗口退出

## 与原项目的差异

| 项目 | 开发板 | PC 模拟器 |
|------|--------|----------|
| 显示 | TLI + LCD | SDL2 窗口 |
| 触摸 | IIC 触摸屏 | 鼠标 |
| 存储 | SD 卡 + FATFS | 本地文件系统 |
| 图片 | SDRAM 缓存 | 堆内存 |
| LED | GPIO 指示灯 | 控制台输出 |
| 延迟 | delay_us() | Sleep() |

## 局限性

- 仅模拟 UI 层（LVGL），不涉及真实硬件外设
- 图片加载需手动准备 .bin 文件
- 无法测试 TLI/DMA/SDRAM 等硬件相关功能
