#pragma once

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include <cstdint>
#include "lvgl.h"

namespace UI {

/**
 * GaugeCard —— 仪表卡片基类
 *
 * 卡片是 UI 编排的最小单位，可被 LayoutEngine 放置在屏幕的任意矩形区域内。
 *
 * 设计约束：
 * - 卡片只读全局数据（latestData / IMU 等），不写
 * - 卡片不知道自己在哪个屏幕、在第几位（bounds 说了算）
 * - 卡片自管 LVGL 对象生命周期（onMount 创建，onUnmount 销毁/隐藏）
 * - 卡片之间不互相通信（保持解耦，避免隐式依赖）
 *
 * 圆屏单卡布局：1 个 GaugeCard 占满 480×480
 * 长条屏多卡布局：N 个 GaugeCard 横向排列
 */
class GaugeCard {
public:
    virtual ~GaugeCard() = default;

    // ============ 生命周期 ============

    /**
     * 挂载到布局：创建 LVGL 对象（首次）或显示已创建对象
     * @param parent  父容器（通常是 lv_scr_act()）
     * @param bounds  分配给本卡片的矩形区域（屏幕坐标）
     */
    virtual void onMount(lv_obj_t* parent, const lv_area_t& bounds) = 0;

    /**
     * 从布局移除：隐藏或销毁 LVGL 对象
     * 注意：onUnmount 不要求销毁，只要"不再可见"即可，保留对象以便快速切回
     */
    virtual void onUnmount() = 0;

    /**
     * 数据 / 视觉更新（按 preferredUpdateMs() 节流调度）
     * 通常在此读取 latestData，写入 LVGL 对象
     */
    virtual void update() = 0;

    // ============ 显示状态钩子（可选） ============

    /** 卡片成为当前可见卡时调用（如 HorizonCard 需要启用特殊渲染模式） */
    virtual void onShow() {}

    /** 卡片不再可见时调用（与 onShow 配对） */
    virtual void onHide() {}

    // ============ 触摸事件（可选） ============

    /**
     * 单击事件
     * @param local_x, local_y  相对卡片左上角的坐标
     * @return true 表示事件已处理，不继续传给布局层
     */
    virtual bool onTap(int16_t local_x, int16_t local_y) {
        (void)local_x; (void)local_y;
        return false;
    }

    /** 长按事件（典型用途：G-Ball 切换量程、Timing 重置峰值） */
    virtual bool onLongPress(int16_t local_x, int16_t local_y) {
        (void)local_x; (void)local_y;
        return false;
    }

    // ============ 元数据 ============

    /** 卡片名称（调试 / 设置菜单显示） */
    virtual const char* name() const = 0;

    /**
     * 期望的 update() 调用间隔（毫秒）
     * - IMU 本地数据卡：25ms（40 FPS）
     * - OBD 数据卡：400ms（匹配 OBD 数据到达频率）
     * - 静态/低频卡：1000ms+
     */
    virtual uint16_t preferredUpdateMs() const { return 400; }

    // ============ 访问器 ============

    const lv_area_t& bounds() const { return bounds_; }
    lv_obj_t* container() const { return container_; }

protected:
    lv_obj_t* container_ = nullptr;
    lv_area_t bounds_{};
};

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
