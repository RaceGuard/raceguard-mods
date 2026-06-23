// RaceGuard mods 主入口模板
//
// 一个典型的车载仪表盘 firmware 大概就这么个骨架. 你可以直接拿去改, 也可以
// 把它当成参考然后自己重写 main.
//
// 链路:
//   raceguard::hal::platform::initHardware()    硬件初始化 (由核心库处理)
//   raceguard::hal::display::startRefresh()     启动显示
//   raceguard::obd::init()                      启动 OBD 后端
//   raceguard::car::registerProfile(YOUR_CAR)   注册你的车型 profile
//   主循环: poll OBD → UI 更新 → 告警检查
//
// v0.0.x: 核心库还没发布, 本文件目前不能 build. v0.1 发布后:
//   1. git clone raceguard-mods
//   2. ./scripts/fetch_core.sh v0.1.0
//   3. cp examples/cars/nissan_gtr_r35/profile.cpp src/app/  (或自己写)
//   4. pio run -e round-led-21 -t upload

#include <Arduino.h>

#include <raceguard/version.h>
#include <raceguard/log.h>
#include <raceguard/hal.h>
#include <raceguard/obd.h>
#include <raceguard/data.h>
#include <raceguard/alert.h>
#include <raceguard/car.h>
#include <raceguard/storage.h>

// 用户在 examples/cars/<品牌_车型>/profile.cpp 中定义并提供
// extern void registerUserCarProfile();

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

    raceguard::hal::display::startRefresh();
    raceguard::storage::init();
    raceguard::obd::init();
    raceguard::alert::init();

    // 注册你的车型 (示例: GT-R)
    // registerUserCarProfile();

    RG_LOG_INFO("Setup complete.");
}

void loop() {
    raceguard::hal::touch::update();

    // OBD 轮询: 数据写入 raceguard::data::latest()
    raceguard::obd::poll(raceguard::data::latest());

    // 告警检查
    auto sev = raceguard::alert::check(raceguard::data::latest());
    if (sev != raceguard::alert::Severity::NONE) {
        // UI 弹告警 (核心库处理)
    }

    // UI 更新: 调用本仓 src/ui/common/ 框架 (PngGaugeCard 等)
    // TODO v0.1: 这里调用 raceguard_mods::ui::update();
    //            目前 UI 框架还没暴露统一入口

    delay(2);
}
