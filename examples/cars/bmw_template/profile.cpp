// BMW 起步模板 — 给 BMW 用户做 mod 适配的起点
//
// 这个模板 *不* 限制任何 PID, 让 SDK 尝试所有 SAE J1979 标准 PID.
// BMW 大部分车型 (E9x F3x G2x) 通过国际通用 OBD-II Mode 01-09 都能读
// 标准参数 (RPM/速度/水温/STFT/AFR/voltage 等), 这套模板已能直接跑.
//
// 你需要做的:
//   1. 复制本文件到你的 mods 仓 src/app/, 改 namespace 为你的车型
//   2. 实车跑一段, 把日志里 NO DATA 的 PID 在 kUnsupported[] 里标出来
//   3. (可选) 抓 INPA / ISTA 出的厂家 P1xxx 故障码, 在 dtcDescribe() 加 case
//   4. (可选) gauges_extra.cpp 启用 OIL_TEMP / BOOST 等默认 disabled 的表

#include <raceguard/car.h>
#include <array>
#include <cstdint>

namespace raceguard_examples::bmw_template {

namespace {

// ============================================================
// PID bitmap — 默认全开
// ============================================================
//
// 写法 A (当前): 全 0xFF, 让 SDK 试所有 PID
// 写法 B: 按 GT-R example 模式, 显式关闭不支持的 PID. 等你实车日志验证后再切换.
//
// 提示: BMW 大部分车型支持 0x5C OIL_TEMP / 0x33 BAROMETRIC_PRESSURE, 比 GT-R 多.
//       但 N20/N55 等涡轮引擎 boost pressure 走 BMW 厂家 PID (Mode 22), 通用 SDK 读不到.

constexpr std::array<uint8_t, 32> buildBitmap() {
    std::array<uint8_t, 32> b{};
    for (auto& v : b) v = 0xFF;
    // TODO: 实车验证后, 把 NO DATA 的 PID 在这里关掉:
    // constexpr uint8_t kUnsupported[] = { 0xXX, 0xYY };
    // for (uint8_t pid : kUnsupported) {
    //     b[pid >> 3] &= ~(1 << (7 - (pid & 7)));
    // }
    return b;
}

constexpr auto kBitmap = buildBitmap();

// ============================================================
// DTC 描述 — 空 stub
// ============================================================
//
// 返 nullptr → SDK 走内置 SAE J2012 标准表 fallback.
// 实车跑出 P1xxx / U1xxx 厂家码时, 在这加 case.
// BMW 厂家码可查: INPA / ISTA-D / DIS GT1 / 论坛 (Bimmerfest, E90Post).

const char* dtcDescribe(uint16_t code) {
    // 提取数字部分 (高 2 位是类型: 00=P, 01=C, 10=B, 11=U)
    uint8_t type = (code >> 14) & 0x03;
    uint16_t num = code & 0x3FFF;

    if (type == 0x00) {     // P (Powertrain)
        switch (num) {
            // TODO: 加你的 BMW P1xxx 厂家码
            // 示例:
            // case 0x1000: return "ECU 内部错误 (示例)";
            default: break;
        }
    } else if (type == 0x03) {  // U (Network)
        switch (num) {
            // TODO: 加 BMW K-CAN / PT-CAN 通信码
            default: break;
        }
    }
    return nullptr;
}

}  // namespace

// ============================================================
// Profile 实例
// ============================================================

const raceguard::car::Profile kProfile = {
    /* .name                      = */ "BMW (Template)",
    /* .supported_pids_bitmap     = */ kBitmap.data(),
    /* .supported_pids_bitmap_len = */ kBitmap.size(),
    /* .dtc_describe              = */ dtcDescribe,
};

void registerProfile() {
    raceguard::car::registerProfile(kProfile);
}

}  // namespace raceguard_examples::bmw_template
