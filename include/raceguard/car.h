#pragma once

#include <cstdint>
#include <cstddef>

namespace raceguard::car {

// 车型 profile - 用户在 examples/cars/<品牌_车型>/ 中定义并注册.
struct Profile {
    const char* name;                              // "Nissan GT-R R35"

    // 支持的 PID bitmap (Mode 01)
    const uint8_t* supported_pids_bitmap;          // 32 字节 (PID 0x00-0xFF)
    size_t          supported_pids_bitmap_len;

    // DTC 码描述查询函数 (返回静态字符串, 不需调用方释放)
    const char* (*dtc_describe)(uint16_t code);

    // 可选: PID 调度优先级覆盖 (nullptr 走 SDK 默认调度)
    // 后续版本可加: const uint8_t* pid_priority_p0; size_t p0_count; ...
};

// 注册车型 profile. 通常在 main.cpp setup() 中调用一次.
void registerProfile(const Profile& profile);

// 获取当前生效的 profile (没注册过返回 nullptr).
const Profile* getActiveProfile();

}  // namespace raceguard::car
