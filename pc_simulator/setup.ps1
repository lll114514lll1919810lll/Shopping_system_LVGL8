# setup.ps1 - Shopping System LVGL PC 模拟器一键构建脚�?# 用法: powershell -ExecutionPolicy Bypass -File setup.ps1

$ErrorActionPreference = "Stop"
$RootDir = Split-Path -Parent $MyInvocation.MyCommand.Path

Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Shopping System LVGL PC Simulator" -ForegroundColor Cyan
Write-Host " 环境准备 & 构建" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ============================================================
# 1. 检�?CMake
# ============================================================
Write-Host "[1/4] 检�?CMake..." -ForegroundColor Yellow
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "ERROR: 未找�?CMake。请�?https://cmake.org/download/ 安装" -ForegroundColor Red
    exit 1
}
Write-Host "  cmake: $($cmake.Source)" -ForegroundColor Green

# ============================================================
# 2. 检�?安装 SDL2
# ============================================================
Write-Host "[2/4] 检�?SDL2..." -ForegroundColor Yellow

$SDL2_Found = $false
$SDL2_DIR = ""

# 方式1: 检�?vcpkg
$vcpkg_sdl = "$env:USERPROFILE\vcpkg\installed\x64-windows\lib\SDL2.lib"
if (Test-Path $vcpkg_sdl) {
    Write-Host "  找到 vcpkg SDL2: $vcpkg_sdl" -ForegroundColor Green
    $SDL2_Found = $true
    $SDL2_DIR = "$env:USERPROFILE\vcpkg\installed\x64-windows"
}

# 方式2: 检查系统安�?if (-not $SDL2_Found) {
    $sys_sdl = "C:\SDL2\lib\x64\SDL2.lib"
    if (Test-Path $sys_sdl) {
        Write-Host "  找到 C:\SDL2" -ForegroundColor Green
        $SDL2_Found = $true
        $SDL2_DIR = "C:\SDL2"
    }


# 方式3: 自动下载 vcpkg �?SDL2
if (-not $SDL2_Found) {
    Write-Host "  未找�?SDL2，正在通过 vcpkg 自动安装..." -ForegroundColor Yellow

    $vcpkg_root = "$env:USERPROFILE\vcpkg"
    if (-not (Test-Path "$vcpkg_root\vcpkg.exe")) {
        Write-Host "  克隆 vcpkg..." -ForegroundColor Yellow
        git clone https://github.com/Microsoft/vcpkg.git $vcpkg_root
        & "$vcpkg_root\bootstrap-vcpkg.bat"
    }

    Write-Host "  安装 SDL2..." -ForegroundColor Yellow
    & "$vcpkg_root\vcpkg.exe" install sdl2:x64-windows --triplet x64-windows

    if (Test-Path "$vcpkg_root\installed\x64-windows\lib\SDL2.lib") {
        Write-Host "  SDL2 安装完成" -ForegroundColor Green
        $SDL2_Found = $true
        $SDL2_DIR = "$vcpkg_root\installed\x64-windows"
    } else {
        Write-Host "ERROR: SDL2 安装失败" -ForegroundColor Red
        exit 1
    }
}

# ============================================================
# 3. 准备 sim_data 目录
# ============================================================
Write-Host "[3/4] 准备模拟数据目录..." -ForegroundColor Yellow

$SimData = Join-Path $RootDir "sim_data"
if (-not (Test-Path $SimData)) {
    New-Item -ItemType Directory -Path $SimData | Out-Null
    Write-Host "  创建 $SimData" -ForegroundColor Green
}

# 创建空的配置文件（如果不存在�?$empty_files = @("transactions.csv", "coupon.dat", "price.dat")
foreach ($f in $empty_files) {
    $path = Join-Path $SimData $f
    if (-not (Test-Path $path)) {
        New-Item -ItemType File -Path $path | Out-Null
        Write-Host "  创建空文�?$f" -ForegroundColor Green
    }
}

Write-Host "  提示: �?SD 卡中�?.bin 图片文件复制�?sim_data/ 即可显示商品图片" -ForegroundColor Cyan

# ============================================================
# 4. CMake 配置 & 构建
# ============================================================
Write-Host "[4/4] CMake 配置 & 构建..." -ForegroundColor Yellow

$BuildDir = Join-Path $RootDir "build"
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

Push-Location $BuildDir

# ?? vcpkg toolchain ???? CI ?????
$vcpkg_toolchain = ""
if ($SDL2_DIR) {
    # ? SDL2_DIR ?? vcpkg root????vcpkg/installed/x64-windows?
    $vcpkg_candidate = Split-Path (Split-Path $SDL2_DIR)
    $toolchain = Join-Path $vcpkg_candidate "scripts\buildsystems\vcpkg.cmake"
    if (Test-Path $toolchain) {
        $vcpkg_toolchain = "-DCMAKE_TOOLCHAIN_FILE=$toolchain"
    }
}

Write-Host "  cmake $vcpkg_toolchain .."
cmd /c "cmake $vcpkg_toolchain .. 2>&1"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake 配置失败" -ForegroundColor Red
    Pop-Location
    exit 1
}

Write-Host "  cmake --build . --config Release"
cmd /c "cmake --build . --config Release 2>&1"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: 构建失败" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " 构建成功!" -ForegroundColor Green
Write-Host ""
Write-Host " 运行: .\build\Release\shopping_sim.exe" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
