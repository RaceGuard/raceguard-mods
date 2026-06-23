// RaceGuard mods 主入口
//
// 一个最小骨架, 直接拿去改即可. 全部业务逻辑在 raceguard::backend 内部:
//   ACTIVATED → 全功能 (OBD/SD/告警/导航/电源/UI)
//   DEMO      → UI 框架 + mock 数据驱动 (空白 ESP32 可烧机演示)
//
// 烧到任何 ESP32 都能进 DEMO 看到 UI 卡片动画; 商业盒子 NVS 预写激活码自动 ACTIVATED.
// 设置菜单点 ACTIVATE → 手机连 WiFi 输 16 位激活码 → 重启进 ACTIVATED.
//
// 快速上手:
//   1. git clone raceguard-mods
//   2. ./scripts/fetch_core.sh v0.1.1-dev   (拉预编译 .a)
//   3. pio run -e round-led-21 -t upload

#include <Arduino.h>

#include <raceguard/version.h>
#include <raceguard/log.h>
#include <raceguard/hal.h>
#include <raceguard/backend.h>

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

    raceguard::hal::display::setBrightness(0);   // 等 UI 第一帧再亮
    raceguard::backend::startAll();
}

void loop() {
    raceguard::backend::tick();
}
