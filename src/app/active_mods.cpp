// active_mods.cpp — 启用 mod 的集中注册点
//
// 用户加 car / theme mod 时, 在这里取消注释对应行启用.
// main.cpp 自动调 register_active_mods() (在 backend::startAll() 之前), 你不要改 main.cpp.
//
// 时序保证:
//   register_active_mods()  ← 这里 (车型 profile + 主题 register)
//   backend::startAll()     ← 接住注册好的 profile, OBDManager::init / theme::reloadFromNVS
//
// 例: 启用 Nissan GT-R R35 车型 mod + Dark Minimal 主题
//   1. ./scripts/new_car.sh ...    OR  确认 examples/cars/nissan_gtr_r35 已编进 build_src_filter
//   2. 取消下面对应行的注释
//   3. pio run -e round-led-21 -t upload

#include "active_mods.h"

// ============ 车型 mod forward declaration ============
//
// 把你 enable 的车型 mod namespace 声明在这, 跟下面 register_active_mods() body 里
// 取消注释的 register 调用配对.

// namespace raceguard_examples::nissan_gtr_r35 { void registerProfile(); }
// namespace raceguard_examples::bmw_template   { void registerProfile(); void enableExtraGauges(); }
// namespace raceguard_examples::honda_civic_fk7 { void registerProfile(); }  // 你新加的车型 mod

// ============ 主题 mod forward declaration (覆盖默认主题, 一般用 FS 主题不需要这) ============

// 注: v0.2.0+ 推荐用 raceguard::ui::theme::select(...) + LittleFS 切主题, 不需要在这注册.
// 这里的 registerTheme 是老式 registerGauges 覆盖路径, 给特殊需求用.

// ============ 注册 (用户编辑, 取消注释对应行) ============

void register_active_mods() {
    // 车型 mod (一次只能 enable 一个, 后注册的覆盖前面):
    // raceguard_examples::nissan_gtr_r35::registerProfile();
    // raceguard_examples::bmw_template::registerProfile();

    // 车型 mod 的额外配置 (启用默认 disabled 的仪表表等):
    // raceguard_examples::bmw_template::enableExtraGauges();   // 必须在 backend::startAll() 之后调

    // (空 — 默认不启用任何 mod, 跑 stock 通用 OBD-II)
}
