#pragma once

// v0.8.0+：LED 圆屏已迁移到 PngGaugeCard，本卡片暂保留供 P4 后续接入
#if defined(DISPLAY_TYPE_P4_BAR)

#include "gauge_card.h"
#include "../lv_compat.h"
#include <cmath>

namespace UI {

/**
 * GaugeMeterCard —— 通用可配置仪表卡片
 *
 * 视觉:
 * - 240° 主弧（深灰底板，7:00 → 3:00）
 * - 33 个外侧刻度线（大-小-半-小-大循环，glow 层仅大/半高）
 * - 5 个刻度值标签（每 2 大刻度一个）
 * - 三层发光长指针 + 中心轴盖
 * - LCD 风格视窗弧（米色，120°）+ 数字 + 单位 + MIN/MAX
 * - 标题长条（视窗右上）
 *
 * 数据源: gauge_params.h 配置的多个 GaugeParamID
 * 触发: 长按循环切换参数（NVS 延迟持久化）
 *
 * 几何/字号自适应: 所有半径/位置基于 bounds 计算
 */
class GaugeMeterCard : public GaugeCard {
public:
    GaugeMeterCard();
    ~GaugeMeterCard() override;

    void onMount(lv_obj_t* parent, const lv_area_t& bounds) override;
    void onUnmount() override;
    void update() override;
    bool onLongPress(int16_t local_x, int16_t local_y) override;

    const char* name() const override { return "GAUGE"; }
    uint16_t preferredUpdateMs() const override { return 25; }

private:
    static constexpr int MAJOR_DIVS    = 8;
    static constexpr int TICKS_PER_DIV = 4;
    static constexpr int TOTAL_TICKS   = MAJOR_DIVS * TICKS_PER_DIV + 1;
    static constexpr int LABEL_EVERY_N = 2;
    static constexpr int LABEL_COUNT   = MAJOR_DIVS / LABEL_EVERY_N + 1;
    static constexpr int GLOW_TICK_COUNT = (MAJOR_DIVS + 1) + MAJOR_DIVS;

    // ============ 几何 ============
    int16_t cx_ = 0, cy_ = 0;
    int16_t r_outer_ = 0;
    float arc_r_       = 0;
    float arc_w_       = 0;
    float tick_outer_r_= 0;
    float tick_major_len_ = 0;
    float tick_half_len_  = 0;
    float tick_small_len_ = 0;
    float label_r_     = 0;
    float needle_tip_r_= 0;
    float needle_tail_r_= 0;
    int16_t hub_outer_size_ = 0;
    int16_t hub_inner_size_ = 0;
    int16_t lcd_val_off_x_  = 0;
    int16_t lcd_val_off_y_  = 0;
    int16_t lcd_unit_x_     = 0;
    int16_t lcd_unit_y_     = 0;
    int16_t lcd_mm_off_x_   = 0;
    int16_t lcd_mm_off_y_   = 0;
    int16_t lcd_title_x_    = 0;
    int16_t lcd_title_y_    = 0;
    int16_t lcd_title_w_    = 0;
    int16_t lcd_title_h_    = 0;
    int16_t tick_unit_x_    = 0;
    int16_t tick_unit_y_    = 0;

    // ============ 字号 ============
    const lv_font_t* font_value_      = nullptr;
    const lv_font_t* font_unit_       = nullptr;
    const lv_font_t* font_title_      = nullptr;
    const lv_font_t* font_minmax_     = nullptr;
    const lv_font_t* font_tick_label_ = nullptr;
    const lv_font_t* font_tick_unit_  = nullptr;

    // ============ LVGL 对象 ============
    lv_obj_t* arc_       = nullptr;
    lv_obj_t* val_label_ = nullptr;
    lv_obj_t* unit_label_= nullptr;
    lv_obj_t* mm_label_  = nullptr;
    lv_obj_t* lcd_panel_ = nullptr;
    lv_obj_t* tick_unit_label_ = nullptr;
    lv_obj_t* title_bar_ = nullptr;
    lv_obj_t* title_label_= nullptr;
    lv_obj_t* needle_glow_ = nullptr;
    lv_obj_t* needle_body_ = nullptr;
    lv_obj_t* needle_core_ = nullptr;
    lv_obj_t* needle_hub_  = nullptr;
    lv_obj_t* tick_glows_[GLOW_TICK_COUNT] = {};
    lv_obj_t* tick_lines_[TOTAL_TICKS]     = {};
    lv_obj_t* tick_labels_[LABEL_COUNT]    = {};
    int       glow_to_tick_[GLOW_TICK_COUNT] = {};
    LinePoint tick_pts_[TOTAL_TICKS][2];

    // ============ 数据状态 ============
    int       param_id_       = 0;     // GaugeParamID 值
    float     session_min_    = NAN;
    float     session_max_    = NAN;
    float     needle_pct_smooth_ = 0.0f;

    LinePoint needle_pts_[2] = {{0,0},{0,0}};

    // ============ NVS 延迟写入 ============
    bool     nvs_dirty_ = false;
    uint32_t nvs_dirty_time_ = 0;

    // ============ 私有方法 ============
    void selectGeometryAndFonts();
    void createObjects(lv_obj_t* parent);
    void destroyObjects();
    void arcPoint(float deg, float r, lv_coord_t& ox, lv_coord_t& oy) const;
    void updateNeedle(float pct);
    void updateTickColors();
    void syncLabels();
    void scheduleNVSWrite();
    void flushNVSIfDirty();
    static int loadParamFromNVS();
    static int firstEnabled(int start);
};

}  // namespace UI

#endif  // DISPLAY_TYPE_P4_BAR
