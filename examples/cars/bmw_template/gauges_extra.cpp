// BMW 模板: 启用 .a 默认 disabled 的仪表 (OIL_TEMP / BOOST)
//
// SDK 内置的 8 张通用 PNG 仪表里:
//   ✓ COOLANT / RPM / SPEED / VOLTS / INTAKE / AFR — 默认 enabled
//   ✗ OIL_TEMP / BOOST — 默认 disabled (因 GT-R 不支持 0x5C / 0x0B)
//
// BMW 大部分车型支持这两个 PID, 启用它们让长按轮换能切到这两张表.
//
// 用法: 把本文件 cp 到你 mods 仓 src/app/, 在 main.cpp 的 backend::startAll()
//       之后调一次 enableExtraGauges().

#include <raceguard/ui.h>
#include <cstdint>
#include <cstring>

namespace raceguard_examples::bmw_template {

// 按名字查仪表 idx, 启用/禁用
static void enableGaugeByName(const char* target, bool enabled) {
    const uint8_t total = raceguard::ui::gaugeCount();
    for (uint8_t i = 0; i < total; ++i) {
        const char* name = raceguard::ui::gaugeName(i);
        if (name && std::strcmp(name, target) == 0) {
            raceguard::ui::setGaugeEnabled(i, enabled);
            return;
        }
    }
}

void enableExtraGauges() {
    // BMW 一般支持的两张表 (主仓默认关因 GT-R 不支持)
    enableGaugeByName("OIL_TEMP", true);
    enableGaugeByName("BOOST",    true);

    // 反向 demo (如果你不想看某张默认开的表):
    // enableGaugeByName("AFR", false);
}

}  // namespace raceguard_examples::bmw_template
