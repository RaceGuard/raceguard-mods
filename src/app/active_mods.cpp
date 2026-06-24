// active_mods.cpp — 启用 mod 的集中注册点 (由 ./scripts/enable_mod.sh 维护)
//
// 推荐流程:
//   ./scripts/new_car.sh honda_civic_fk7        # 生成 examples/cars/honda_civic_fk7/
//   ./scripts/enable_mod.sh cars honda_civic_fk7 # 一条命令改 3 处 (本文件 + platformio.ini)
//   ./scripts/flash_all.sh                      # 烧 firmware + LittleFS
//
// 关闭全部 mod 回 stock 通用 OBD-II:
//   ./scripts/enable_mod.sh disable
//
// 时序保证:
//   main.cpp setup() → register_active_mods() (这里) → backend::startAll()
//   backend 接住注册好的 profile, OBDManager::init / theme::reloadFromNVS 走预设
//
// 主题 mod 不需要在这注册. v0.2.0+ 走 LittleFS + raceguard::ui::theme::select(...),
// uploadfs 后从设置菜单切.

#include "active_mods.h"

// ============ AUTO-GENERATED 块 ============
// >>> ENABLE_MOD 标记之间的内容由 ./scripts/enable_mod.sh 重写, 不要手改.
// 标记本身不要删, 删了脚本定位不到.

// >>> ENABLE_MOD:DECL_BEGIN
// <<< ENABLE_MOD:DECL_END

void register_active_mods() {
    // >>> ENABLE_MOD:CALL_BEGIN
    // <<< ENABLE_MOD:CALL_END
}
