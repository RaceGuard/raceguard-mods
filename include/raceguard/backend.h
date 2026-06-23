#pragma once

namespace raceguard::backend {

// 启动全部后端子系统:
//   - 时间同步 (TimeSync)
//   - SD 卡 / 异步日志写入 (SDLogger / LogWriter), DEMO 模式跳过
//   - BLE 协议栈 + OBD/NAV 后端启动 (BLEScanner / NavReceiver / NavANCS), DEMO 模式跳过
//   - CAN 后端硬件初始化 (CommHAL::initCAN), CAN 模式 only
//   - 电源管理 (PowerManager), 触摸处理 (TouchHandler), 串口命令 (SerialCmd)
//   - WiFi 文件服务初始化
//
// 内部已根据 raceguard::license::isActivated() 自动分流:
//   ACTIVATED → 全功能启动 (上述全部)
//   DEMO      → 仅 PowerManager / TouchHandler / SerialCmd, 跳过 OBD/BLE/Nav/SD 等闭源功能
//
// 调用时序: 必须在 raceguard::ui::init() 之后, raceguard::ui::finishBoot() 之前调用
//   (BLE 协议栈大量 PSRAM 分配, DMA 启动后还要 BLE 状态变化时重启 DMA)
//
// 返回: 是否启动成功 (false 表示某关键子系统 init 失败, 但 firmware 仍能跑)
bool startAll();

// 主循环每次调用, 内部驱动:
//   - 触摸轮询 + UI 帧渲染 (= raceguard::ui::update + hal::touch::update)
//   - OBD/BLE/Nav 状态轮询 (ACTIVATED 模式)
//   - 告警检测 + 弹窗 (ACTIVATED 模式)
//   - 日志写入定时 / OBD 断连防抖 / 待机检测 (ACTIVATED 模式)
//   - BLE 状态边沿监测 + DMA 重启防偏移 (ACTIVATED 模式)
//   - 自适应延时 (OBD 活跃 2ms / 未连接 50ms / 待机 100ms)
//
// 用户主循环可以仅一行 raceguard::backend::tick(), 也可以拆开调底层 API 自行组装。
void tick();

}  // namespace raceguard::backend
