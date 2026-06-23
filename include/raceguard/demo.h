#pragma once

namespace raceguard::demo {

// 启动 mock OBD 后台 task, 周期性 (100Hz) 写 raceguard::data::latest() 各字段,
// 正弦波摆动覆盖典型量程 (RPM/Speed/水温/AFR/Timing/FuelTrim 等), 驱动 UI 仪表卡片动画.
//
// 适用场景: 设备未激活 (DEMO 模式) 时给 UI 提供数据源.
// 幂等 — 重复调用不会创建多个 task.
void startMockFeed();

}  // namespace raceguard::demo
