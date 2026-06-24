// Nissan GT-R R35 完整车型适配示例
//
// 演示 raceguard::car::Profile 的两个核心字段:
//   1. supported_pids_bitmap — 让 OBD 调度只查 ECU 真的支持的 PID, 减少超时浪费
//   2. dtc_describe          — 厂家 P1xxx / U1xxx 故障码描述, fallback 到 SAE 标准
//
// GT-R 实车验证 (2026): RPM / timing / STFT / coolant / AFR / voltage / 速度
// 等 SAE J1979 标准 PID 全部 OK; 0x66 新型 MAF / 0x49/0x4A 油门 / 0x3C/0x3D
// 催化温度 / 0x43 绝对负荷也回数据. 详见主仓 docs/reference/PID_REFERENCE.md.

#include <raceguard/car.h>
#include <array>
#include <cstdint>

namespace raceguard_examples::nissan_gtr_r35 {

// ============================================================
// 1. 支持的 PID bitmap
// ============================================================
//
// 格式: 32 字节, 与 SAE J1979 Service 0x01 PID 0x00/0x20/... 返回值一致
//   byte 0 bit 7 = PID 0x00, bit 6 = PID 0x01, ..., bit 0 = PID 0x07
//   byte 1            = PID 0x08..0x0F
//   ...
//   byte 31           = PID 0xF8..0xFF
//
// 实战做法 (更可读): 默认全开, 把已知不支持的 PID 显式关掉.

namespace {

constexpr std::array<uint8_t, 32> buildBitmap() {
    std::array<uint8_t, 32> b{};
    for (auto& v : b) v = 0xFF;     // 默认全开

    // GT-R R35 实测不支持的 PID (Mode 01 返回 NO DATA):
    constexpr uint8_t kUnsupported[] = {
        0x0A,   // FUEL_PRESSURE
        0x0B,   // INTAKE_MANIFOLD_ABS_PRESSURE  (改用 0x33/timing 估算 boost)
        0x10,   // MAF_RATE                       (GT-R 用 0x66 新型 MAF 替代)
        0x2F,   // FUEL_LEVEL                     (GT-R 不报)
        0x33,   // BAROMETRIC_PRESSURE
        0x46,   // AMBIENT_AIR_TEMP
        0x5C,   // ENGINE_OIL_TEMP                (Mode 01 不支持; Consult III+ 可读)
    };
    for (uint8_t pid : kUnsupported) {
        b[pid >> 3] &= ~(1 << (7 - (pid & 7)));
    }
    return b;
}

constexpr auto kBitmap = buildBitmap();

// ============================================================
// 2. DTC 描述 — 厂家 P1xxx / U1xxx
// ============================================================
//
// 注: GT-R 厂家故障码列表非常长 (R35 Service Manual EC/AT section 列了 ~200 个),
//     这里只放几个最常见的作演示, 完整列表请参考:
//       - R35 Service Manual EC section
//       - Consult III+ 软件 DTC 索引
//       - nicoclub.com / GT-R Life 论坛
//
// 返回 nullptr → SDK 走 .a 内 SAE J2012 标准表 fallback.

const char* dtcDescribe(uint16_t code) {
    // 提取数字部分 (高 2 位是类型: 00=P, 01=C, 10=B, 11=U)
    uint8_t type = (code >> 14) & 0x03;
    uint16_t num = code & 0x3FFF;

    if (type == 0x00) {     // P
        switch (num) {
            case 0x1212: return "VDC 系统液压压力低";
            case 0x1217: return "VR38 引擎过热保护 (水温/油温超限)";
            case 0x1320: return "点火信号 (主线圈输出)";
            case 0x1715: return "ATTESA-ETS 前驱扭矩分配异常";
            case 0x1720: return "AT 油温过高保护";
            case 0x1805: return "刹车开关电路";
            default: break;
        }
    } else if (type == 0x03) {  // U (网络/通信)
        switch (num) {
            case 0x1000: return "CAN 总线通信故障";
            case 0x1001: return "CAN 总线 BCM 通信丢失";
            case 0x1218: return "CAN 总线 TCU 通信丢失";
            default: break;
        }
    }
    return nullptr;   // fallback 到 SAE 标准表
}

}  // namespace

// ============================================================
// 3. Profile 实例
// ============================================================

const raceguard::car::Profile kProfile = {
    /* .name                      = */ "Nissan GT-R R35",
    /* .supported_pids_bitmap     = */ kBitmap.data(),
    /* .supported_pids_bitmap_len = */ kBitmap.size(),
    /* .dtc_describe              = */ dtcDescribe,
};

// ============================================================
// 4. 注册入口 (user 在 main.cpp setup() 调一次)
// ============================================================

void registerProfile() {
    raceguard::car::registerProfile(kProfile);
}

}  // namespace raceguard_examples::nissan_gtr_r35
