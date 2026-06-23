#pragma once

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "gauge_card.h"
#include "../lv_compat.h"

namespace UI {

/**
 * DigitalCard —— 4 行数字仪表卡片
 *
 * 内容: COOLANT / INTAKE / AFR(双数: lambda×14.7 + lambda×100) / BATTERY
 * 布局: 4 行竖排，每行 = 红色横条+标题 + 大数字+单位
 * 圆屏特性: 标题/单位用弦计算贴合圆边（ShapeKind::CIRCLE）
 *          矩形屏改用 padding（ShapeKind::RECT）
 *
 * 数据源: latestData.coolant_temp / intake_temp / air_fuel_ratio / battery_voltage
 * 触发: 无
 *
 * 几何/字号自适应: 行高/字号/横条宽按 bounds 计算
 * 字号档位（自定义数字字体 + Montserrat fallback）:
 *   bounds 短边 ≥400px: lv_font_digital_96 / _80 / _64 + unit_32（圆屏 480 等价）
 *   bounds 短边 300-400: digital_80 / _64 + unit_32（中等）
 *   bounds 短边 <300: Montserrat fallback
 */
class DigitalCard : public GaugeCard {
public:
    DigitalCard();
    ~DigitalCard() override;

    void onMount(lv_obj_t* parent, const lv_area_t& bounds) override;
    void onUnmount() override;
    void update() override;

    const char* name() const override { return "DIGITAL"; }
    uint16_t preferredUpdateMs() const override { return 0; }  // 不节流（LVGL 文本相等自动 no-op）

    // ============ 区段对象（public 供文件内 free 函数访问） ============
    struct Section {
        lv_obj_t* value  = nullptr;
        lv_obj_t* value2 = nullptr;   // 仅 AFR
        lv_obj_t* unit   = nullptr;
    };

private:
    // ============ 几何 ============
    int16_t bar_h_         = 0;
    int16_t val_h_         = 0;
    int16_t section_pitch_ = 0;
    int16_t bar_w_         = 0;
    int16_t title_inset_   = 0;
    int16_t unit_inset_    = 0;
    int16_t afr_val_gap_   = 0;
    int16_t afr_unit_gap_  = 0;

    // ============ 字号 ============
    const lv_font_t* font_value_main_  = nullptr;   // COOLANT/INTAKE 主数值
    const lv_font_t* font_value_bat_   = nullptr;   // BATTERY（防底部溢出）
    const lv_font_t* font_value_afr2_  = nullptr;   // AFR 副数 (×100)
    const lv_font_t* font_unit_        = nullptr;   // 单位
    const lv_font_t* font_title_       = nullptr;   // 红条标题

    Section clt_{};
    Section iat_{};
    Section afr_{};
    Section bat_{};

    // ============ 工具 ============
    /** 弦计算：CIRCLE 返回 y 处半弦长，RECT 返回 bounds_w/2 */
    int16_t chordHalfAt(int16_t y_local) const;

    void selectGeometryAndFonts();
    void createObjects(lv_obj_t* parent);
    void destroyObjects();

    void makeBarAndTitle(lv_obj_t* parent, const char* title, int16_t bar_center_y);
    void makeSection(lv_obj_t* parent, Section& s, const char* title,
                     int16_t bar_center_y, const lv_font_t* val_font, bool has_unit);
    void makeAfrSection(lv_obj_t* parent, Section& s, int16_t bar_center_y);
};

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
