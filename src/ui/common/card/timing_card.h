#pragma once

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "gauge_card.h"
#include "../lv_compat.h"
#include <cmath>

namespace UI {

/**
 * TimingCard —— 点火角仪表卡片（半圆指针 + 极值弧 + 大数字 + 副数据）
 *
 * 量程: -10° ~ +40° (50° span)
 * 视觉: 180° 半圆 meter + EMA 平滑指针 + 蓝色极值 sweep arc + 红光边
 *      + 中央大数字 + 引擎负载 bar + 水温/电压副标签
 *
 * 数据源: latestData.timing_advance / engine_load / coolant_temp / battery_voltage
 * 触发: 长按重置峰值
 *
 * 几何/字号自适应: 所有位置/半径/字号按 bounds 计算，无 480/240 字面量
 * 圆屏 480×480 视觉与旧 LEDPageTiming 等价
 */
class TimingCard : public GaugeCard {
public:
    TimingCard();
    ~TimingCard() override;

    void onMount(lv_obj_t* parent, const lv_area_t& bounds) override;
    void onUnmount() override;
    void update() override;
    bool onLongPress(int16_t local_x, int16_t local_y) override;

    const char* name() const override { return "TIMING"; }
    uint16_t preferredUpdateMs() const override { return 25; }

    /** 长按调用 / 外部 API: 清零会话极值 */
    void resetPeaks();

private:
    // ============ 几何 (bounds 自适应) ============
    int16_t cx_           = 0;   // 卡片中心 X (屏幕坐标)
    int16_t cy_           = 0;   // 卡片中心 Y
    int16_t r_outer_      = 0;   // 卡片内切圆半径
    int16_t meter_size_   = 0;   // lv_meter widget 边长
    int16_t needle_tip_r_ = 0;
    int16_t needle_tail_r_= 0;
    int16_t sweep_size_   = 0;   // 极值弧 size (= 2 × sweep_r)
    int16_t bar_w_        = 0;
    int16_t bar_h_        = 0;
    int16_t value_y_off_  = 0;   // 大数字相对中心 Y 偏移
    int16_t minmax_y_off_ = 0;
    int16_t bar_y_off_    = 0;
    int16_t load_label_y_off_ = 0;
    int16_t bottom_pad_   = 0;   // 底部 label 距底 padding
    int16_t bottom_side_pad_ = 0; // 底部 label 距侧 padding

    // ============ 字号档位 ============
    const lv_font_t* font_value_      = nullptr;   // 大数字
    const lv_font_t* font_minmax_     = nullptr;
    const lv_font_t* font_secondary_  = nullptr;   // 水温/电压
    const lv_font_t* font_label_      = nullptr;   // ENGINE LOAD
    const lv_font_t* font_tick_       = nullptr;   // meter 刻度

    // ============ LVGL 对象 ============
    lv_obj_t* meter_         = nullptr;
    lv_obj_t* needle_glow_   = nullptr;
    lv_obj_t* needle_body_   = nullptr;
    lv_obj_t* needle_core_   = nullptr;
    lv_obj_t* needle_hub_outer_ = nullptr;
    lv_obj_t* needle_hub_inner_ = nullptr;
    lv_obj_t* sweep_arc_     = nullptr;
    lv_obj_t* value_label_   = nullptr;
    lv_obj_t* minmax_label_  = nullptr;
    lv_obj_t* bar_           = nullptr;
    lv_obj_t* coolant_label_ = nullptr;
    lv_obj_t* vbat_label_    = nullptr;

    // ============ 数据状态 ============
    float session_min_ = NAN;
    float session_max_ = NAN;
    float needle_pct_smooth_ = 0.0f;

    // 变化检测（避免不必要的 LVGL 文本/样式更新）
    char    last_value_str_[24]   = "";
    char    last_minmax_str_[48]  = "";
    int16_t last_bar_val_         = -999;
    char    last_coolant_str_[16] = "";
    char    last_vbat_str_[16]    = "";

    // 指针端点（相对坐标，避免全屏 invalidation）
    // LVGL 9 用 lv_point_precise_t, LVGL 8 用 lv_point_t (lv_compat.h 收敛)
    LinePoint needle_pts_[2] = {{0,0},{0,0}};

    // ============ 私有方法 ============
    void selectGeometryAndFonts();
    void createObjects(lv_obj_t* parent);
    void destroyObjects();
    void addEdgeGlow(lv_obj_t* parent);     // 圆屏边缘装饰光环（bounds 自适应）
    void updateNeedle(float pct);
    void updateSweepArc(float vmin, float vmax);
};

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
