#pragma once

#include "../../src/types/car_data.h"

namespace raceguard::alert {

enum class Severity : int8_t {
    NONE = -1,
    P0   = 0,   // Critical (爆震 / 失火 / 过温)
    P1   = 1,   // Warning  (STFT 持续 / 油温高)
    P2   = 2,   // Notice
    P3   = 3,   // Info
};

bool init();

// 检查当前数据是否触发告警. 返回最严重等级, 没触发返回 NONE.
Severity check(const CarData& data);

}  // namespace raceguard::alert
