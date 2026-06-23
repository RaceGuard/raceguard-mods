#pragma once

// 默认只在 P4 编译（P4 没接 OBD 之前的主数据源）；LED 平台可临时启用做 FPS 压测
#if defined(DISPLAY_TYPE_P4_BAR) || defined(LED_MOCK_OBD_FEED)

/**
 * mock_obd_feed —— P4 平台的临时 OBD 数据模拟器
 *
 * P4 BLE 链路 (Phase 2) 接入之前, 卡片需要有数据才能验证视觉.
 * 本模块启动一个 FreeRTOS 后台 task, 200ms 周期写 latestData 各字段,
 * 数值用正弦/三角波摆动覆盖典型量程, 便于直观验证仪表动画.
 *
 * latestData 定义放在 mock_obd_feed.cpp, 因为它就是当前 P4 唯一数据源.
 * 等真 OBD 接入后, 定义会迁移到 OBD driver, 本模块整体删除.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 启动 mock OBD 后台 task. 幂等 (重复调用不会创建多个 task).
 * 调用前 latestData 已全零初始化, 调用后 ~200ms 第一组数据出现.
 */
void mock_obd_feed_start(void);

#ifdef __cplusplus
}
#endif

#endif  // DISPLAY_TYPE_P4_BAR || LED_MOCK_OBD_FEED
