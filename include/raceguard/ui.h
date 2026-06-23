#pragma once

#include <cstdint>

namespace raceguard::ui {

// 启动图形子系统: LVGL 初始化 + 仪表 UI 控件创建 + 显示开机画面.
// 必须在 hal::display::startRefresh() 之后调用.
bool init();

// 完成开机过渡: 切换到上次保存的仪表页, 渐亮回放.
// 若设备处于 DEMO 模式 (license::current()==DEMO), 额外弹一次激活提示卡 (3 秒自动关).
// 必须在 ui::init() 之后调用.
void finishBoot();

// 主循环每次调用: LVGL 帧处理 + UI 数据刷新 + 触摸事件分发.
// 自带帧率限流, 调用频率无需精确.
void update();

// 进入设置菜单 (等同于用户上/下边缘滑动手势).
void enterSettings();

// ============ 仪表轮换自定义 ============
// 通用仪表卡片 (CARD_GAUGE) 内置 8 张表 (COOLANT/OIL_TEMP/RPM/SPEED/VOLTS/INTAKE/BOOST/AFR),
// 长按仪表卡片轮换. 默认 COOLANT/RPM/SPEED/VOLTS/INTAKE/AFR 启用,
// OIL_TEMP/BOOST 因 GT-R PID 不支持默认关闭.
//
// mods 用户两种自定义方式:
//
//   方式 1 (轻量): 用 setGaugeEnabled 开关默认 8 张表里的某几张
//     在 backend::startAll() 之后调用 (PngGaugeCard 已创建).
//
//   方式 2 (深度): 用 registerGauges 注册全新的仪表清单 (自己的 PNG + getter)
//     必须在 backend::startAll() 之前调用 (否则被 default kGauges 覆盖).
//     注册的清单完全替换 default, mods 用户负责选用合适的 PNG 资产和 getter 函数.

// ----- 方式 1: 运行期开关 -----

// 仪表总数 (含默认 disabled 的)
uint8_t gaugeCount();

// 第 idx 张表的名字 (字面量, e.g. "COOLANT" "RPM"); 越界返回 nullptr
const char* gaugeName(uint8_t idx);

// 运行期开关单张仪表 (长按轮换跳过 disabled; 关闭当前显示的表自动切下一张 enabled)
void setGaugeEnabled(uint8_t idx, bool enabled);

// 查询单张仪表当前 enabled 状态
bool isGaugeEnabled(uint8_t idx);

// ----- 方式 2: 注册自定义仪表清单 -----
//
// 类型用 UI::PngGaugeCard::Def (开源 SDK, 见 src/ui/common/card/png_gauge_card.h)
// 包含: name / image (LVGL 底图) / value 量程 / 角度量程 / getter+has_value / fmt / enabled_default
//
// 例 (mods 仓 src/app/main.cpp):
//   #include "ui/common/card/png_gauge_card.h"
//   #include "my_gauge_assets.h"   // 自己 bake 的 PNG
//   static const UI::PngGaugeCard::Def myGauges[] = {
//       { "RPM_CUSTOM", &my_rpm_png, 0, 12000, -180, 45, getRpm, hasRpm, "%.0f", true },
//       ...
//   };
//   void setup() {
//       ...
//       raceguard::ui::registerGauges(myGauges, sizeof(myGauges)/sizeof(myGauges[0]));
//       raceguard::backend::startAll();
//   }
//
// 注: 这个函数声明用 void* 隐藏 Def 类型 (LVGL 类型依赖 mods 仓 lvgl.h 已 install).
//     调用方传 const UI::PngGaugeCard::Def* 强制 cast 为 void* 即可, 内部还原.
void registerGauges(const void* defs, uint8_t count);

}  // namespace raceguard::ui
