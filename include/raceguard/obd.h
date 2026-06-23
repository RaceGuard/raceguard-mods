#pragma once

#include <raceguard/car_data.h>

namespace raceguard::obd {

enum class Backend : uint8_t {
    NONE        = 0,
    BLE_ELM327  = 1,
    CAN_DIRECT  = 2,
};

// 启动 OBD 子系统. 根据 NVS 中保存的 backend 选项初始化对应后端.
bool init();

Backend getActiveBackend();

// 是否已连接到车辆 (BLE: ELM327 BLE 已配对 + ATI 成功; CAN: TWAI 已就绪)
bool isConnected();

// 主循环每次调用. 内部驱动状态机, 解析到的字段写入 out.
void poll(CarData& out);

// 主动断开连接 (设置菜单中切换 backend 时调用)
void disconnect();

}  // namespace raceguard::obd
