#pragma once

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "gauge_card.h"
#include "../lv_compat.h"
#include <cmath>

namespace UI {

/**
 * FuelTrimCard —— 双 Bank 燃油修正 + Lambda 仪表卡片
 *
 * 视觉:
 * - 外侧两个红色 STFT 弧 (B1 左 / B2 右, ±25% symmetrical)
 * - 内侧两个蓝色 LTFT 极值弧 (Bank 1/2 长期峰值范围)
 * - 中央 Lambda 半圆 meter (0.85 ~ 1.15)，发光环装饰
 * - Lambda 三层发光指针 + 中央轴盖
 * - STFT 外刻度线 (±25, ±20, ±15, ±10, ±5, 0)
 * - 数值标签: STFT% / LTFT max/min / Lambda / AFR / Bank 标签
 *
 * 数据源: latestData.short/long_fuel_trim_b1/b2 + air_fuel_ratio
 * 触发: 无
 *
 * 几何/字号自适应: 所有半径/位置/字号按 bounds 计算
 */
class FuelTrimCard : public GaugeCard {
public:
    FuelTrimCard();
    ~FuelTrimCard() override;

    void onMount(lv_obj_t* parent, const lv_area_t& bounds) override;
    void onUnmount() override;
    void update() override;

    const char* name() const override { return "FUEL_TRIM"; }
    uint16_t preferredUpdateMs() const override { return 25; }

private:
    // ============ 几何 (基于 bounds) ============
    int16_t cx_           = 0, cy_ = 0;
    int16_t r_outer_      = 0;
    int16_t stft_size_    = 0;   // STFT 弧 size
    int16_t stft_width_   = 0;
    int16_t ltft_size_    = 0;   // LTFT 弧 size
    int16_t ltft_width_   = 0;
    int16_t lambda_size_  = 0;   // Lambda meter size
    int16_t lambda_tip_r_ = 0;
    int16_t lambda_tail_r_= 0;
    int16_t stft_offset_  = 0;   // Bank 1/2 水平偏移 (-/+)
    int16_t stft_tick_r_  = 0;   // 刻度外径
    int16_t bank_label_off_x_ = 0; // Bank 标签水平 ±xOff
    int16_t bank_label_off_y_ = 0;
    int16_t bank_value_off_y_ = 0;
    int16_t bank_ltft_off_y_  = 0;
    int16_t lambda_label_y_   = 0;
    int16_t afr_label_y_      = 0;
    int16_t lambda_icon_x_    = 0;
    int16_t lambda_icon_y_    = 0;
    int16_t afr_icon_x_       = 0;
    int16_t afr_icon_y_       = 0;
    int16_t hub_outer_size_   = 0;
    int16_t hub_inner_size_   = 0;

    // ============ 字号 ============
    const lv_font_t* font_stft_val_   = nullptr;   // STFT 大数字
    const lv_font_t* font_ltft_val_   = nullptr;   // LTFT 副数
    const lv_font_t* font_lambda_val_ = nullptr;   // Lambda 数值
    const lv_font_t* font_afr_val_    = nullptr;
    const lv_font_t* font_bank_lbl_   = nullptr;
    const lv_font_t* font_tick_       = nullptr;

    // ============ LVGL 对象 ============
    lv_obj_t* stft_arc_b1_ = nullptr;
    lv_obj_t* stft_arc_b2_ = nullptr;
    lv_obj_t* ltft_arc_b1_ = nullptr;
    lv_obj_t* ltft_arc_b2_ = nullptr;
    lv_obj_t* lambda_glow_ = nullptr;
    lv_obj_t* lambda_body_ = nullptr;
    lv_obj_t* lambda_core_ = nullptr;
    lv_obj_t* lambda_hub_  = nullptr;
    lv_obj_t* stft_b1_label_ = nullptr;
    lv_obj_t* stft_b2_label_ = nullptr;
    lv_obj_t* ltft_b1_label_ = nullptr;
    lv_obj_t* ltft_b2_label_ = nullptr;
    lv_obj_t* lambda_label_  = nullptr;
    lv_obj_t* afr_label_     = nullptr;

    // ============ 数据状态 ============
    float ltft_b1_min_ = 0, ltft_b1_max_ = 0;
    float ltft_b2_min_ = 0, ltft_b2_max_ = 0;
    float stft_b1_smooth_   = 0.0f;
    float stft_b2_smooth_   = 0.0f;
    float lambda_pct_smooth_= 50.0f;

    int16_t last_arc_b1_val_ = 0;
    int16_t last_arc_b2_val_ = 0;
    char last_stft_b1_str_[24] = "";
    char last_stft_b2_str_[24] = "";
    char last_ltft_b1_str_[24] = "";
    char last_ltft_b2_str_[24] = "";
    char last_lambda_str_[24]  = "";
    char last_afr_str_[24]     = "";
    uint16_t last_ltft_b1_ang_[2] = {0, 0};
    uint16_t last_ltft_b2_ang_[2] = {0, 0};

    // ============ 指针端点 ============
    // LVGL 9 用 lv_point_precise_t, LVGL 8 用 lv_point_t (lv_compat.h 收敛)
    LinePoint lambda_pts_[2] = {{0,0},{0,0}};

    // ============ 私有方法 ============
    void selectGeometryAndFonts();
    void createObjects(lv_obj_t* parent);
    void destroyObjects();
    void updateLambdaNeedle(float pct);
};

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
