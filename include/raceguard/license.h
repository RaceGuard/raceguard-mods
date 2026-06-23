#pragma once

#include <cstdint>

namespace raceguard::license {

// 设备授权状态
enum class Mode : uint8_t {
    ACTIVATED = 0,   // 全功能解锁
    DEMO      = 1,   // UI 框架 + mock 数据可跑, OBD/SD/告警/导航等闭源功能锁定
};

// 检查 NVS / SD 上的授权码并设置 mode. 不阻塞, 不启 WiFi.
// 设备启动早期 (在 OBD/SD 等子系统 init 之前) 调用一次.
bool init();

// 当前运行模式
Mode current();

// 便捷判断: current() == ACTIVATED
bool isActivated();

// 本机芯片 ID 字符串 (大写十六进制, 12 字符, 无分隔符), 例如 "AABBCCDDEEFF".
// init() 后可用. 返回值指向内部 static buffer.
const char* chipId();

// 启动 WiFi AP 激活流程 (阻塞, 永不返回):
//   暂停 LVGL → DisplayHAL 直绘提示页 → 启 WiFi softAP (192.168.4.1)
//   → 浏览器输入 16 位激活码 → 校验通过 → 写 NVS → ESP.restart()
// 适用场景: 设置菜单点 "ACTIVATE" / 开机 demo 提示卡点跳转
// 注意: 必须在 LVGL 已初始化后调用 (会先暂停 LVGL)
void enterActivationPage();

}  // namespace raceguard::license
