// 默认 P4 编译（提供 latestData 主存储）；LED 加 -DLED_MOCK_OBD_FEED 可启用做 FPS 压测
#if defined(DISPLAY_TYPE_P4_BAR) || defined(LED_MOCK_OBD_FEED)

#include "mock_obd_feed.h"
#include <raceguard/car_data.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <cmath>

static const char *TAG = "MOCK_OBD";

#if defined(DISPLAY_TYPE_P4_BAR)
// ============ latestData 实际存储 (P4 当前唯一数据源) ============
// LED 平台 latestData 在 main.cpp:65 / AppGlobals 单例定义，本模块只 extern 写入
static CarData s_p4_data{};
CarData& latestData = s_p4_data;
#else
// LED：使用 globals.h 暴露的全局引用
#include "../types/globals.h"
#endif

// ============ mock 数据生成 ============

static void mock_obd_task(void* /*arg*/) {
    uint32_t tick = 0;
    while (true) {
        // 所有 8 张表数据扫满量程，频率减慢让指针扫到端点时不被跳过
        // tick * 10ms = 时间，0.012 × tick(rad) ≈ 0.012 rad/10ms = 1.2 rad/s → 周期 ~5s
        // RPM 0..8000（表量程 0-8000）
        latestData.rpm = (uint16_t)(4000.0f + 4000.0f * sinf(tick * 0.012f));

        // Speed 0..340 (表量程 0-340)
        latestData.vehicle_speed = (uint8_t)(170.0f + 170.0f * sinf(tick * 0.016f));

        // Coolant 60..140 °C（表量程 60-140）
        latestData.coolant_temp = 100.0f + 40.0f * sinf(tick * 0.008f);

        // Oil temp 60..150（表量程 60-150）
        latestData.oil_temp = 105.0f + 45.0f * sinf(tick * 0.009f);

        // Intake -20..80（表量程 -20-80）
        latestData.intake_temp = 30.0f + 50.0f * sinf(tick * 0.010f);

        // Battery 10..16V（表量程 10-16）
        latestData.battery_voltage = 13.0f + 3.0f * sinf(tick * 0.020f);

        // AFR 10..18 lambda（表量程 10-18）
        latestData.air_fuel_ratio = 14.0f + 4.0f * sinf(tick * 0.014f);

        // Boost -1..2 bar（表量程 -1-2），boost_pressure 单位 kPa → 乘 100
        latestData.boost_pressure = (0.5f + 1.5f * sinf(tick * 0.018f)) * 100.0f;

        // 其余卡片用的字段（Timing / FuelTrim 等）
        latestData.timing_advance = 15.0f + 25.0f * sinf(tick * 0.16f);
        latestData.engine_load = 55.0f + 35.0f * sinf(tick * 0.12f);
        latestData.short_fuel_trim_b1 = 5.0f * sinf(tick * 0.18f);
        latestData.short_fuel_trim_b2 = 5.0f * cosf(tick * 0.18f);
        latestData.long_fuel_trim_b1  = 2.0f * sinf(tick * 0.08f);
        latestData.long_fuel_trim_b2  = 2.0f * cosf(tick * 0.08f);
        latestData.throttle_pos = 40.0f + 40.0f * sinf(tick * 0.14f);

        tick++;
        vTaskDelay(pdMS_TO_TICKS(10));    // 100 Hz 数据更新，确保 UI 每帧都有新数据可画（FPS 压测）
    }
}

extern "C" void mock_obd_feed_start(void) {
    static bool started = false;
    if (started) {
        ESP_LOGW(TAG, "mock_obd_feed_start 已启动过, 忽略重复调用");
        return;
    }
    started = true;
    BaseType_t r = xTaskCreate(mock_obd_task, "mock_obd", 4096, nullptr, 5, nullptr);
    if (r != pdPASS) {
        ESP_LOGE(TAG, "mock_obd task 创建失败: %d", (int)r);
        started = false;
        return;
    }
    ESP_LOGI(TAG, "mock OBD task 启动 (200ms 周期, RPM/Speed/Timing/FuelTrim 正弦摆动)");
}

#endif  // DISPLAY_TYPE_P4_BAR || LED_MOCK_OBD_FEED
