#pragma once

#include <raceguard/car_data.h>

namespace raceguard::data {

// 当前最新 OBD 数据.
CarData& latest();

// 本次会话峰值 (重启或 OBD 重连时重置).
SessionPeaks& sessionPeaks();

}  // namespace raceguard::data
