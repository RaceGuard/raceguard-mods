// RaceGuard mods 主入口 — 用户一般不需要改这个文件
//
// 加 car / theme mod 改 active_mods.cpp 即可 (集中注册点), 不动这里.
//
// 全部业务逻辑在 raceguard::backend 内部:
//   ACTIVATED → 全功能 (OBD/SD/告警/导航/电源/UI)
//   DEMO      → UI 框架 + mock 数据驱动 (空白 ESP32 可烧机演示)
//
// 烧到任何 ESP32 都能进 DEMO 看到 UI 动画; 商业盒子 NVS 预写激活码自动 ACTIVATED.
// 设置菜单点 ACTIVATE → 手机连 WiFi 输 16 位激活码 → 重启进 ACTIVATED.
//
// 快速上手:
//   1. git clone raceguard-mods
//   2. ./scripts/fetch_core.sh        (读 CORE_VERSION 文件 pin 的版本)
//   3. pio run -e round-led-21 -t upload

#include <Arduino.h>

#include <raceguard/version.h>
#include <raceguard/log.h>
#include <raceguard/hal.h>
#include <raceguard/backend.h>

#include "active_mods.h"

void setup() {
    raceguard::log::init(115200);
    RG_LOG_INFO("===========================================");
    RG_LOG_INFO("RaceGuard mods v%s", raceguard::CORE_VERSION);
    RG_LOG_INFO("Platform: %s", raceguard::hal::platform::getName());
    RG_LOG_INFO("===========================================");

    if (!raceguard::hal::platform::initHardware()) {
        RG_LOG_ERROR("Hardware init failed; halting.");
        while (true) delay(1000);
    }

    register_active_mods();                      // mods (车型 profile + 主题) 注册
    raceguard::hal::display::setBrightness(0);   // 等 UI 第一帧再亮
    raceguard::backend::startAll();              // 接住 mods, 启 OBD/UI/告警
}

void loop() {
    raceguard::backend::tick();
}
