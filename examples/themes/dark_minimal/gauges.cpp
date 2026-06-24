// Dark Minimal 主题包 skeleton — 演示 raceguard::ui::registerGauges 覆盖默认 8 张表
//
// 这是 *骨架* — 真正能跑需要先用 tools/gauge_bakery 烘焙 PNG 底图,
// 然后把生成的 .c 文件 (含 lv_img_dsc_t) 加到 build, 让 extern 声明能链接到.
//
// 完整步骤见同目录 README.md.

#include <raceguard/ui.h>
#include <raceguard/car_data.h>

// PngGaugeCard 类型完整定义在 mods 仓 src/ui/common/card/png_gauge_card.h
// (跟主仓 src/ui/common/ 是同源 sync, 不属于闭源 .a, 你可以直接 include)
#include "ui/common/card/png_gauge_card.h"

// LVGL image descriptor 类型
#include <lvgl.h>

namespace raceguard_examples::dark_minimal {

// ============================================================
// 1. 烘焙产物 — 引用 (烘焙脚本会在 assets/ 下生成同名 .c)
// ============================================================
//
// 用 tools/gauge_bakery 烘焙后, 每张表会生成一个 lv_img_dsc_t 全局变量,
// 命名约定 `gauge_<name>_480`. extern 声明它们让本 TU 能引用.

extern "C" {
    extern const lv_img_dsc_t gauge_dark_coolant_480;
    extern const lv_img_dsc_t gauge_dark_rpm_480;
    extern const lv_img_dsc_t gauge_dark_speed_480;
    extern const lv_img_dsc_t gauge_dark_volts_480;
    extern const lv_img_dsc_t gauge_dark_intake_480;
    extern const lv_img_dsc_t gauge_dark_afr_480;
    extern const lv_img_dsc_t gauge_dark_oil_temp_480;
    extern const lv_img_dsc_t gauge_dark_boost_480;
}

// ============================================================
// 2. CarData getter / has_value (跟主仓默认表一致, 复用最稳)
// ============================================================
namespace {

float getCoolant(const CarData& d)    { return d.coolant_temp; }
bool  hasCoolant(const CarData& d)    { return CarData::hasValue(d.coolant_temp); }
float getRpm(const CarData& d)        { return (float)d.rpm; }
bool  hasRpm(const CarData& d)        { return CarData::hasValue(d.rpm); }
float getSpeed(const CarData& d)      { return (float)d.vehicle_speed; }
bool  hasSpeed(const CarData& d)      { return CarData::hasValue(d.vehicle_speed); }
float getVolts(const CarData& d)      { return d.battery_voltage; }
bool  hasVolts(const CarData& d)      { return CarData::hasValue(d.battery_voltage); }
float getIntake(const CarData& d)     { return d.intake_temp; }
bool  hasIntake(const CarData& d)     { return CarData::hasValue(d.intake_temp); }
float getAfr(const CarData& d)        { return d.air_fuel_ratio; }
bool  hasAfr(const CarData& d)        { return CarData::hasValue(d.air_fuel_ratio); }
float getOilTemp(const CarData& d)    { return d.oil_temp; }
bool  hasOilTemp(const CarData& d)    { return CarData::hasValue(d.oil_temp); }
float getBoost(const CarData& d)      { return d.boost_pressure; }
bool  hasBoost(const CarData& d)      { return CarData::hasValue(d.boost_pressure); }

// ============================================================
// 3. Def[] 数组 — 覆盖默认 8 张表的视觉, 数据源 / 阈值不变
// ============================================================

const UI::PngGaugeCard::Def kGauges[] = {
    // name,      image,                       min,    max,    angle_start,   angle_end,  getter,    has_value,  fmt,      enabled_default
    {"COOLANT",   &gauge_dark_coolant_480,     60.0f,  130.0f, -180.0f,        45.0f,     getCoolant, hasCoolant, "%.0f°C", true },
    {"RPM",       &gauge_dark_rpm_480,         0.0f,   9000.0f,-180.0f,        45.0f,     getRpm,     hasRpm,     "%.0f",   true },
    {"SPEED",     &gauge_dark_speed_480,       0.0f,   320.0f, -180.0f,        45.0f,     getSpeed,   hasSpeed,   "%.0f",   true },
    {"VOLTS",     &gauge_dark_volts_480,       10.0f,  16.0f,  -180.0f,        45.0f,     getVolts,   hasVolts,   "%.1fV",  true },
    {"INTAKE",    &gauge_dark_intake_480,      -20.0f, 80.0f,  -180.0f,        45.0f,     getIntake,  hasIntake,  "%.0f°C", true },
    {"AFR",       &gauge_dark_afr_480,         10.0f,  20.0f,  -180.0f,        45.0f,     getAfr,     hasAfr,     "%.1f",   true },
    {"OIL_TEMP",  &gauge_dark_oil_temp_480,    60.0f,  150.0f, -180.0f,        45.0f,     getOilTemp, hasOilTemp, "%.0f°C", false},   // 默认关
    {"BOOST",     &gauge_dark_boost_480,       -50.0f, 200.0f, -180.0f,        45.0f,     getBoost,   hasBoost,   "%.0fkPa",false},   // 默认关
};
constexpr uint8_t kGaugeCount = sizeof(kGauges) / sizeof(kGauges[0]);

}  // namespace

// ============================================================
// 4. 注册入口 (user 在 main.cpp setup 调一次)
//
//   ⚠️ 必须在 raceguard::ui::init() 之前调
//      (init 时构造 PngGaugeCard, 之后注册晚了 SDK 会跳过)
//      raceguard::ui::init() 由 backend::startAll() 内部调,
//      所以 registerTheme() 必须在 backend::startAll() 之前.
// ============================================================

void registerTheme() {
    raceguard::ui::registerGauges(kGauges, kGaugeCount);
}

}  // namespace raceguard_examples::dark_minimal
