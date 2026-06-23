#pragma once

#include <cstdint>
#include <cstddef>

namespace raceguard::hal {

// ============ 显示 ============
namespace display {

bool init();                     // 创建 panel + 分配帧缓冲, DMA 未启动
void startRefresh();             // 启动 RGB / DSI DMA (二阶段启动第二阶段)
void setBrightness(uint8_t v);   // 0-255
void restartDMA();               // BLE 大动作后重启 DMA 消除 PSRAM 争用偏移

}  // namespace display


// ============ 触摸 ============
namespace touch {

struct State {
    int16_t x, y;
    bool isHolding;       // 当前按住
    bool wasPressed;      // 本次轮询期间 down 边沿
    bool wasReleased;     // 本次轮询期间 up 边沿
};

State get();
void update();            // 主循环调用, 轮询底层 IC

}  // namespace touch


// ============ IMU (LED 平台 G-Ball 用; P4 无 IMU 时 isAvailable() 返回 false) ============
namespace imu {

struct Accel { float x, y, z; };  // g

bool isAvailable();
Accel read();

}  // namespace imu


// ============ NVS 持久化 ============
namespace nvs {

bool getU32(const char* ns, const char* key, uint32_t& out);
bool setU32(const char* ns, const char* key, uint32_t val);
bool getStr(const char* ns, const char* key, char* out, size_t max_len);
bool setStr(const char* ns, const char* key, const char* val);

}  // namespace nvs


// ============ 平台总入口 ============
namespace platform {

bool initHardware();           // 内部按二阶段顺序调 display::init + I2C + IMU + 触摸 IC 等
const char* getName();         // "ESP32-S3 LED 21" / "ESP32-P4 6.2-inch DSI" / ...

}  // namespace platform

}  // namespace raceguard::hal
