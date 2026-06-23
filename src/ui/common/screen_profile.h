#pragma once

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include <cstdint>
#include "lvgl.h"

namespace UI {

enum class ShapeKind : uint8_t {
    RECT,            // 矩形屏（长条屏 / 一般 LCD）
    CIRCLE,          // 圆形屏（当前 480×480 圆 LCD）
    ROUNDED_RECT,    // 圆角矩形
    OCTAGON,         // 八边形（异型）
    CUSTOM_MASK,     // 自定义形状，alpha mask
};

/**
 * ScreenProfile —— 屏幕几何 / 形状 / 字号档位的集中描述
 *
 * 设计目标：把"屏幕长什么样"这件事从 UI 代码里剥离出来。
 * UI 代码引用 currentProfile() 提供的语义信息（safe area / 字号档位 / 形状判定），
 * 不再硬编码 480 / 240 / 圆心 / 半径等具体数值。
 *
 * 加新屏 = 新增一份 profile 实例 + 链接到 currentProfile()
 */
struct ScreenProfile {
    // ============ 物理尺寸 ============
    uint16_t width;           // 像素
    uint16_t height;          // 像素
    uint16_t dpi;             // 物理 DPI（决定字号选择）

    // ============ 形状 ============
    ShapeKind shape;
    uint16_t corner_radius;       // 仅 ROUNDED_RECT 有效
    const uint8_t* shape_mask;    // 仅 CUSTOM_MASK 有效，可为 nullptr

    // ============ 安全绘制区域 ============
    // 异型屏可见区域矩形，所有 UI 必须在此范围内
    int16_t safe_x;
    int16_t safe_y;
    uint16_t safe_w;
    uint16_t safe_h;

    // ============ 触摸交互 ============
    uint8_t edge_swipe_width;     // 边缘滑动判定宽度（像素）

    // ============ 字号档位 ============
    // UI 代码引用语义档位（xs/sm/md/lg/xl/2xl），不直接绑定具体字号宏
    const lv_font_t* font_xs;     // 14-16 px：单位标签 / 辅助文本
    const lv_font_t* font_sm;     // 16-18 px：副标题 / 按钮文本
    const lv_font_t* font_md;     // 20-24 px：正文 / 中等数值
    const lv_font_t* font_lg;     // 24-32 px：页面标题 / 大数值
    const lv_font_t* font_xl;     // 32-48 px：仪表数值
    const lv_font_t* font_2xl;    // 48+ px：超大数字（Timing 大字 / 全屏 RPM）

    // ============ 工具方法 ============

    /** 判断 (x, y) 是否在屏幕可见区域内（综合考虑形状和 safe area） */
    bool isInsideVisibleArea(int16_t x, int16_t y) const;
};

/**
 * 获取当前平台的屏幕配置
 *
 * 每个 LVGL 显示平台（DISPLAY_TYPE_LED / DISPLAY_TYPE_P4_BAR / ...）
 * 必须提供一个 .cpp 实现此函数，返回 static 的 profile 实例。
 *
 * 当前实现位置：
 * - DISPLAY_TYPE_LED → src/ui/led/screen_profile_led.cpp
 * - DISPLAY_TYPE_P4_BAR → src/ui/p4_bar/screen_profile_p4_bar.cpp (待实现)
 */
const ScreenProfile& currentProfile();

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
