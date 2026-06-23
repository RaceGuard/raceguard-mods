#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "timing_card.h"
#include "../screen_profile.h"
#include <raceguard/data.h>
#include <raceguard/car_data.h>
#include <raceguard/log.h>

#include <cstdio>
#include <cstring>

namespace UI {

// ============ 常量（与旧 LEDPageTiming 等价） ============
static constexpr float PI_F            = 3.14159265f;
static constexpr float SCALE_MIN       = -10.0f;
static constexpr float SCALE_MAX       =  40.0f;
static constexpr float SCALE_RANGE     =  50.0f;
static constexpr float SCALE_DEG_START = 180.0f;   // meter rotation
static constexpr float SCALE_DEG_SPAN  = 180.0f;
static constexpr float EMA_ALPHA       = 0.65f;

// ============ 颜色（inline 函数；lv_color_make 不是 constexpr） ============
static inline lv_color_t cTickMajor()  { return lv_color_make(150, 150, 150); }
static inline lv_color_t cTickMinor()  { return lv_color_make(80, 80, 80); }
static inline lv_color_t cTrack()      { return lv_color_make(51, 51, 51); }
static inline lv_color_t cRedZone()    { return lv_color_make(180, 0, 0); }
static inline lv_color_t cSweep()      { return lv_color_make(0, 100, 255); }
static inline lv_color_t cNeedleGlow() { return lv_color_make(180, 0, 0); }
static inline lv_color_t cNeedleBody() { return lv_color_make(220, 0, 0); }
static inline lv_color_t cNeedleCore() { return lv_color_make(255, 60, 40); }
static inline lv_color_t cHubGlow()    { return lv_color_make(60, 60, 60); }
static inline lv_color_t cHubInner()   { return lv_color_make(25, 25, 25); }
static inline lv_color_t cHubBorder()  { return lv_color_make(180, 0, 0); }
static inline lv_color_t cValueOn()    { return lv_color_make(255, 255, 255); }
static inline lv_color_t cValueOff()   { return lv_color_make(100, 100, 100); }
static inline lv_color_t cMinMax()     { return lv_color_make(0, 100, 255); }
static inline lv_color_t cLabelDim()   { return lv_color_make(150, 150, 150); }
static inline lv_color_t cLabel()      { return lv_color_make(120, 120, 120); }
static inline lv_color_t cBarBg()      { return lv_color_make(40, 40, 40); }
static inline lv_color_t cBarBorder()  { return lv_color_make(80, 80, 80); }

// ============ 辅助 ============
static inline float valToPct(float val) {
    return (val - SCALE_MIN) / SCALE_RANGE * 100.0f;
}

static inline int16_t boundsW(const lv_area_t& b) { return b.x2 - b.x1 + 1; }
static inline int16_t boundsH(const lv_area_t& b) { return b.y2 - b.y1 + 1; }
static inline int16_t shortSide(const lv_area_t& b) {
    int16_t w = boundsW(b), h = boundsH(b);
    return w < h ? w : h;
}

// ============ 构造 / 析构 ============
TimingCard::TimingCard() = default;

TimingCard::~TimingCard() {
    destroyObjects();
}

// ============ 几何 + 字号 ============
void TimingCard::selectGeometryAndFonts() {
    int16_t w = boundsW(bounds_);
    int16_t h = boundsH(bounds_);
    // cx_/cy_ 用 container 局部坐标 (不加 bounds.x1/y1).
    // 圆屏 bounds 总是 (0,0,...) 加不加等价, P4 多卡布局必须不加,
    // 否则子节点 set_pos 给的"屏幕绝对坐标"被当成 container 局部坐标 → 画到容器外被裁.
    cx_ = w / 2;
    cy_ = h / 2;
    int16_t ss = shortSide(bounds_);
    r_outer_ = ss / 2;

    // 几何比例（以旧 480 实现为基准，比例 / 240）
    meter_size_    = (int16_t)(ss * 470L / 480);    // 470/480
    needle_tip_r_  = (int16_t)(ss * 210L / 480);    // 210/240 → 折半 = 210/480 (相对 ss/2)
    needle_tail_r_ = (int16_t)(ss *  20L / 480);
    sweep_size_    = (int16_t)(ss * 252L / 480);
    bar_w_         = (int16_t)(ss * 240L / 480);
    bar_h_         = (int16_t)(ss *  18L / 480);
    value_y_off_   = (int16_t)(ss *  50L / 480);
    minmax_y_off_  = (int16_t)(ss *  90L / 480);
    bar_y_off_     = (int16_t)(ss * 140L / 480);
    load_label_y_off_  = (int16_t)(ss * 115L / 480);
    bottom_pad_       = (int16_t)(h *  55L / 480);
    bottom_side_pad_  = (int16_t)(w * 120L / 480);

    // 字号按 bounds 短边选档
    const ScreenProfile& p = currentProfile();
    if (ss >= 400) {
        font_value_     = p.font_2xl;      // ~48px
        font_minmax_    = p.font_sm;       // ~16px
        font_secondary_ = p.font_md;       // ~20px
        font_label_     = p.font_xs;       // ~14px
        font_tick_      = p.font_sm;       // meter 刻度
    } else if (ss >= 300) {
        font_value_     = p.font_xl;
        font_minmax_    = p.font_xs;
        font_secondary_ = p.font_sm;
        font_label_     = p.font_xs;
        font_tick_      = p.font_xs;
    } else {
        font_value_     = p.font_lg;
        font_minmax_    = p.font_xs;
        font_secondary_ = p.font_xs;
        font_label_     = p.font_xs;
        font_tick_      = p.font_xs;
    }
}

// ============ Edge Glow (bounds 自适应) ============
void TimingCard::addEdgeGlow(lv_obj_t* parent) {
    // 仅在圆屏 / 较大方形卡片下视觉合理
    int16_t ss = shortSide(bounds_);
    if (ss < 300) return;  // 小卡片省略装饰

    struct Layer { int16_t size_off; int16_t width; uint8_t r, g, b; };
    static const Layer kLayers[] = {
        { 0, 6,  80, 18,  8},    // 最外
        { 8, 6,  56, 12,  5},
        {16, 6,  36,  8,  3},
        {24, 6,  18,  4,  2},
        {32, 5,   6,  1,  0},    // 最内
    };
    for (const auto& L : kLayers) {
        int16_t size = ss - L.size_off;
        if (size <= 0) break;
        lv_obj_t* ring = lv_arc_create(parent);
        lv_obj_set_size(ring, size, size);
        lv_obj_align(ring, LV_ALIGN_CENTER, 0, 0);
        lv_arc_set_rotation(ring, 0);
        lv_arc_set_bg_angles(ring, 0, 360);
        lv_arc_set_range(ring, 0, 1);
        lv_arc_set_value(ring, 0);
        lv_obj_set_style_arc_color(ring, lv_color_make(L.r, L.g, L.b), LV_PART_MAIN);
        lv_obj_set_style_arc_width(ring, L.width, LV_PART_MAIN);
        lv_obj_set_style_arc_rounded(ring, false, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(ring, LV_OPA_TRANSP, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, LV_PART_KNOB);
        lv_obj_set_style_pad_all(ring, 0, LV_PART_KNOB);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(ring, 0, LV_PART_MAIN);
        lv_obj_clear_flag(ring, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    }
}

// ============ 创建 LVGL 对象 ============
void TimingCard::createObjects(lv_obj_t* parent) {
    selectGeometryAndFonts();

    // ── container ──
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, boundsW(bounds_), boundsH(bounds_));
    lv_obj_set_pos(container_, bounds_.x1, bounds_.y1);
    lv_obj_set_style_bg_color(container_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_radius(container_, 0, 0);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);

    // ── 边缘装饰光环 ──
    addEdgeGlow(container_);

    // ── 180° 半圆 meter ──
    // 圆屏 LVGL 8 用 lv_meter (有 add_arc indicator 机制),
    // P4 LVGL 9 用 lv_scale + section 替代 (LVGL 9 删除了 lv_meter widget).
    // meter_ 字段类型不变 (lv_obj_t*), 两版本下指向不同 widget 实例.
    int16_t track_width = (int16_t)(r_outer_ * 114L / 240);  // 旧 114 (240 基准)

#if LVGL_VERSION_MAJOR >= 9
    // ─── LVGL 9: lv_scale (ROUND_INNER) + 3 section ───
    meter_ = lv_scale_create(container_);
    lv_obj_set_size(meter_, meter_size_, meter_size_);
    lv_obj_align(meter_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(meter_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(meter_, 0, 0);
    lv_obj_set_style_pad_all(meter_, 5, 0);
    lv_obj_set_style_text_font(meter_, font_tick_, 0);
    lv_obj_set_style_text_color(meter_, cTickMajor(), 0);
    lv_obj_clear_flag(meter_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // 圆形 scale, 180° 半圆, 从 9 点钟(180°) 起, 量程 -10..+40
    lv_scale_set_mode(meter_, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_angle_range(meter_, (uint32_t)SCALE_DEG_SPAN);
    lv_scale_set_rotation(meter_, (int32_t)SCALE_DEG_START);
    lv_scale_set_range(meter_, (int32_t)SCALE_MIN, (int32_t)SCALE_MAX);
    lv_scale_set_total_tick_count(meter_, 11);   // 11 个总刻度 (含端点)
    lv_scale_set_major_tick_every(meter_, 2);    // 每 2 个一个 major
    lv_scale_set_label_show(meter_, false);      // 数字 label 暂不显示

    // 主轨道弧 (PART_MAIN), 灰色背景
    lv_obj_set_style_arc_color(meter_, cTrack(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(meter_, track_width, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(meter_, LV_OPA_COVER, LV_PART_MAIN);

    // tick 样式: major (PART_INDICATOR) / minor (PART_ITEMS)
    lv_obj_set_style_line_color(meter_, cTickMajor(), LV_PART_INDICATOR);
    lv_obj_set_style_length(meter_, 18, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(meter_, 3, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(meter_, cTickMinor(), LV_PART_ITEMS);
    lv_obj_set_style_length(meter_, 10, LV_PART_ITEMS);
    lv_obj_set_style_line_width(meter_, 1, LV_PART_ITEMS);

    // 红区 style (两 section 共用, static 避免重复初始化)
    static lv_style_t style_red_zone;
    static bool style_red_inited = false;
    if (!style_red_inited) {
        lv_style_init(&style_red_zone);
        style_red_inited = true;
    }
    // bounds 变化重建时 width 可能变, 每次都更新 width/color (LVGL style 是引用, OK)
    lv_style_set_arc_color(&style_red_zone, cRedZone());
    lv_style_set_arc_width(&style_red_zone, track_width);
    lv_style_set_arc_opa(&style_red_zone, LV_OPA_COVER);

    // 红区 section 1: -10..0°
    lv_scale_section_t* sec_red_lo = lv_scale_add_section(meter_);
    lv_scale_set_section_range(meter_, sec_red_lo, (int32_t)SCALE_MIN, 0);
    lv_scale_set_section_style_main(meter_, sec_red_lo, &style_red_zone);

    // 红区 section 2: 35..40°
    lv_scale_section_t* sec_red_hi = lv_scale_add_section(meter_);
    lv_scale_set_section_range(meter_, sec_red_hi, 35, (int32_t)SCALE_MAX);
    lv_scale_set_section_style_main(meter_, sec_red_hi, &style_red_zone);

#else
    // ─── LVGL 8: 原 lv_meter (圆屏路径) ───
    meter_ = lv_meter_create(container_);
    lv_obj_set_size(meter_, meter_size_, meter_size_);
    lv_obj_align(meter_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(meter_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(meter_, 0, 0);
    lv_obj_set_style_pad_all(meter_, 5, 0);
    lv_obj_set_style_text_font(meter_, font_tick_, 0);
    lv_obj_set_style_text_color(meter_, cTickMajor(), 0);
    lv_obj_clear_flag(meter_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    lv_meter_scale_t* scale = lv_meter_add_scale(meter_);
    lv_meter_set_scale_ticks(meter_, scale, 11, 1, 10, cTickMinor());
    lv_meter_set_scale_major_ticks(meter_, scale, 2, 3, 18, cTickMajor(), 18);
    lv_meter_set_scale_range(meter_, scale,
                             (int32_t)SCALE_MIN, (int32_t)SCALE_MAX,
                             (uint32_t)SCALE_DEG_SPAN, (uint32_t)SCALE_DEG_START);

    // 背景轨道弧
    lv_meter_indicator_t* track = lv_meter_add_arc(meter_, scale, track_width, cTrack(), 0);
    lv_meter_set_indicator_start_value(meter_, track, (int32_t)SCALE_MIN);
    lv_meter_set_indicator_end_value(meter_, track, (int32_t)SCALE_MAX);

    // 红区（低 < 0°）
    lv_meter_indicator_t* red_lo = lv_meter_add_arc(meter_, scale, track_width, cRedZone(), 0);
    lv_meter_set_indicator_start_value(meter_, red_lo, (int32_t)SCALE_MIN);
    lv_meter_set_indicator_end_value(meter_, red_lo, 0);

    // 红区（高 > 35°）
    lv_meter_indicator_t* red_hi = lv_meter_add_arc(meter_, scale, track_width, cRedZone(), 0);
    lv_meter_set_indicator_start_value(meter_, red_hi, 35);
    lv_meter_set_indicator_end_value(meter_, red_hi, (int32_t)SCALE_MAX);
#endif

    // ── ENGINE LOAD 标签 ──
    lv_obj_t* load_label = lv_label_create(container_);
    lv_label_set_text(load_label, "ENGINE LOAD");
    lv_obj_set_style_text_font(load_label, font_label_, 0);
    lv_obj_set_style_text_color(load_label, cLabel(), 0);
    lv_obj_align(load_label, LV_ALIGN_CENTER, 0, load_label_y_off_);

    // ── 极值弧（独立 lv_arc，避免触发 meter 全 widget invalidation） ──
    sweep_arc_ = lv_arc_create(container_);
    lv_obj_set_size(sweep_arc_, sweep_size_, sweep_size_);
    lv_obj_align(sweep_arc_, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(sweep_arc_, 180);
    lv_arc_set_bg_angles(sweep_arc_, 0, 180);
    lv_arc_set_range(sweep_arc_, 0, 180);
    lv_obj_set_style_arc_opa(sweep_arc_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_color(sweep_arc_, cSweep(), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(sweep_arc_, (int16_t)(r_outer_ * 20L / 240), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(sweep_arc_, false, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sweep_arc_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sweep_arc_, 0, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(sweep_arc_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sweep_arc_, 0, 0);
    lv_obj_clear_flag(sweep_arc_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_arc_set_angles(sweep_arc_, 0, 0);

    // ── 三层发光指针 ──
    int16_t needle_w_glow = (int16_t)(r_outer_ * 15L / 240);
    int16_t needle_w_body = (int16_t)(r_outer_ * 10L / 240);
    int16_t needle_w_core = (int16_t)(r_outer_ *  4L / 240);
    if (needle_w_glow < 5) needle_w_glow = 5;
    if (needle_w_body < 3) needle_w_body = 3;
    if (needle_w_core < 2) needle_w_core = 2;

    needle_glow_ = lv_line_create(container_);
    lv_line_set_points(needle_glow_, needle_pts_, 2);
    lv_obj_set_style_line_color(needle_glow_, cNeedleGlow(), 0);
    lv_obj_set_style_line_width(needle_glow_, needle_w_glow, 0);
    lv_obj_set_style_line_opa(needle_glow_, 80, 0);
    lv_obj_set_style_line_rounded(needle_glow_, true, 0);
    lv_obj_clear_flag(needle_glow_, LV_OBJ_FLAG_CLICKABLE);

    needle_body_ = lv_line_create(container_);
    lv_line_set_points(needle_body_, needle_pts_, 2);
    lv_obj_set_style_line_color(needle_body_, cNeedleBody(), 0);
    lv_obj_set_style_line_width(needle_body_, needle_w_body, 0);
    lv_obj_set_style_line_opa(needle_body_, 180, 0);
    lv_obj_set_style_line_rounded(needle_body_, true, 0);
    lv_obj_clear_flag(needle_body_, LV_OBJ_FLAG_CLICKABLE);

    needle_core_ = lv_line_create(container_);
    lv_line_set_points(needle_core_, needle_pts_, 2);
    lv_obj_set_style_line_color(needle_core_, cNeedleCore(), 0);
    lv_obj_set_style_line_width(needle_core_, needle_w_core, 0);
    lv_obj_set_style_line_rounded(needle_core_, true, 0);
    lv_obj_clear_flag(needle_core_, LV_OBJ_FLAG_CLICKABLE);

    // ── 轴盖（外圈灰底 + 内圈黑芯红边） ──
    int16_t hub_outer = (int16_t)(r_outer_ * 32L / 240);
    int16_t hub_inner = (int16_t)(r_outer_ * 20L / 240);
    if (hub_outer < 16) hub_outer = 16;
    if (hub_inner < 10) hub_inner = 10;

    needle_hub_outer_ = lv_obj_create(container_);
    lv_obj_set_size(needle_hub_outer_, hub_outer, hub_outer);
    lv_obj_align(needle_hub_outer_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(needle_hub_outer_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(needle_hub_outer_, cHubGlow(), 0);
    lv_obj_set_style_bg_opa(needle_hub_outer_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(needle_hub_outer_, lv_color_make(40, 40, 40), 0);
    lv_obj_set_style_border_width(needle_hub_outer_, 2, 0);
    lv_obj_clear_flag(needle_hub_outer_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    needle_hub_inner_ = lv_obj_create(container_);
    lv_obj_set_size(needle_hub_inner_, hub_inner, hub_inner);
    lv_obj_align(needle_hub_inner_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(needle_hub_inner_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(needle_hub_inner_, cHubInner(), 0);
    lv_obj_set_style_bg_opa(needle_hub_inner_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(needle_hub_inner_, cHubBorder(), 0);
    lv_obj_set_style_border_width(needle_hub_inner_, 2, 0);
    lv_obj_clear_flag(needle_hub_inner_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    updateNeedle(needle_pct_smooth_);

    // ── 主数值大字 ──
    value_label_ = lv_label_create(container_);
    lv_label_set_text(value_label_, "---");
    lv_obj_set_style_text_font(value_label_, font_value_, 0);
    lv_obj_set_style_text_color(value_label_, cValueOn(), 0);
    lv_obj_set_style_text_align(value_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(value_label_, (int16_t)(boundsW(bounds_) / 2));
    lv_obj_align(value_label_, LV_ALIGN_CENTER, 0, value_y_off_);

    // ── 极值标签 ──
    minmax_label_ = lv_label_create(container_);
    lv_label_set_text(minmax_label_, "MIN --- / MAX ---");
    lv_obj_set_style_text_font(minmax_label_, font_minmax_, 0);
    lv_obj_set_style_text_color(minmax_label_, cMinMax(), 0);
    lv_obj_set_style_text_align(minmax_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(minmax_label_, (int16_t)(boundsW(bounds_) * 5 / 8));
    lv_obj_align(minmax_label_, LV_ALIGN_CENTER, 0, minmax_y_off_);

    // ── 负载进度条 ──
    bar_ = lv_bar_create(container_);
    lv_bar_set_range(bar_, 0, 100);
    lv_bar_set_value(bar_, 0, LV_ANIM_OFF);
    lv_obj_set_size(bar_, bar_w_, bar_h_);
    lv_obj_align(bar_, LV_ALIGN_CENTER, 0, bar_y_off_);
    lv_obj_set_style_bg_color(bar_, cBarBg(), 0);
    lv_obj_set_style_bg_color(bar_, lv_color_make(255, 0, 0), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_, bar_h_ / 2, 0);
    lv_obj_set_style_pad_all(bar_, 0, 0);
    lv_obj_set_style_border_width(bar_, 2, 0);
    lv_obj_set_style_border_color(bar_, cBarBorder(), 0);

    // ── 底部水温 / 电压 ──
    coolant_label_ = lv_label_create(container_);
    lv_label_set_text(coolant_label_, "--\xC2\xB0""C");
    lv_obj_set_style_text_font(coolant_label_, font_secondary_, 0);
    lv_obj_set_style_text_color(coolant_label_, cLabelDim(), 0);
    lv_obj_align(coolant_label_, LV_ALIGN_BOTTOM_LEFT, bottom_side_pad_, -bottom_pad_);

    vbat_label_ = lv_label_create(container_);
    lv_label_set_text(vbat_label_, "--V");
    lv_obj_set_style_text_font(vbat_label_, font_secondary_, 0);
    lv_obj_set_style_text_color(vbat_label_, cLabelDim(), 0);
    lv_obj_align(vbat_label_, LV_ALIGN_BOTTOM_RIGHT, -bottom_side_pad_, -bottom_pad_);
}

void TimingCard::destroyObjects() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    meter_ = nullptr;
    needle_glow_ = needle_body_ = needle_core_ = nullptr;
    needle_hub_outer_ = needle_hub_inner_ = nullptr;
    sweep_arc_ = nullptr;
    value_label_ = minmax_label_ = nullptr;
    bar_ = nullptr;
    coolant_label_ = vbat_label_ = nullptr;
}

// ============ 指针更新（相对坐标，无全屏 invalidation） ============
void TimingCard::updateNeedle(float pct) {
    float deg = SCALE_DEG_START + pct / 100.0f * SCALE_DEG_SPAN;
    float rad = deg * PI_F / 180.0f;
    float cs  = cosf(rad), sn = sinf(rad);

    float tip_x  = cx_ + needle_tip_r_  * cs;
    float tip_y  = cy_ + needle_tip_r_  * sn;
    float tail_x = cx_ - needle_tail_r_ * cs;
    float tail_y = cy_ - needle_tail_r_ * sn;

    float pad = 8.0f;
    float min_x = fminf(tip_x, tail_x) - pad;
    float min_y = fminf(tip_y, tail_y) - pad;
    lv_coord_t ox = (lv_coord_t)min_x;
    lv_coord_t oy = (lv_coord_t)min_y;

    // 相对坐标（line obj 位置 + 端点）
    needle_pts_[0] = {(lv_coord_t)(tail_x - min_x), (lv_coord_t)(tail_y - min_y)};
    needle_pts_[1] = {(lv_coord_t)(tip_x  - min_x), (lv_coord_t)(tip_y  - min_y)};

    if (!needle_glow_ || !needle_body_ || !needle_core_) return;

    lv_disp_t* d = lv_disp_get_default();
    lv_disp_enable_invalidation(d, false);

    lv_obj_set_pos(needle_glow_, ox, oy);
    lv_obj_set_pos(needle_body_, ox, oy);
    lv_obj_set_pos(needle_core_, ox, oy);
    lv_line_set_points(needle_glow_, needle_pts_, 2);
    lv_line_set_points(needle_body_, needle_pts_, 2);
    lv_line_set_points(needle_core_, needle_pts_, 2);

    lv_disp_enable_invalidation(d, true);
}

// ============ 极值弧角度更新 ============
void TimingCard::updateSweepArc(float vmin, float vmax) {
    if (!sweep_arc_) return;
    float cmin = fmaxf(SCALE_MIN, fminf(SCALE_MAX, vmin));
    float cmax = fmaxf(SCALE_MIN, fminf(SCALE_MAX, vmax));
    uint16_t a_start = (uint16_t)((cmin - SCALE_MIN) / SCALE_RANGE * 180.0f);
    uint16_t a_end   = (uint16_t)((cmax - SCALE_MIN) / SCALE_RANGE * 180.0f);
    if (a_end <= a_start) a_end = a_start + 1;
    lv_arc_set_angles(sweep_arc_, a_start, a_end);
}

// ============ 生命周期 ============
void TimingCard::onMount(lv_obj_t* parent, const lv_area_t& bounds) {
    // 检查是否需要重建（bounds 变化或首次）
    bool needs_rebuild = !container_ ||
                         bounds.x1 != bounds_.x1 || bounds.y1 != bounds_.y1 ||
                         bounds.x2 != bounds_.x2 || bounds.y2 != bounds_.y2;
    bounds_ = bounds;

    if (needs_rebuild) {
        destroyObjects();
        session_min_ = NAN;
        session_max_ = NAN;
        needle_pct_smooth_ = valToPct(0.0f);
        last_value_str_[0] = last_minmax_str_[0] = '\0';
        last_coolant_str_[0] = last_vbat_str_[0] = '\0';
        last_bar_val_ = -999;
        createObjects(parent);
    } else {
        lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }
}

void TimingCard::onUnmount() {
    if (container_) {
        lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }
}

// ============ 数据更新 ============
void TimingCard::update() {
    if (!container_) return;

    const CarData& d = raceguard::data::latest();
    bool has_tim  = CarData::hasValue(d.timing_advance);
    bool has_load = CarData::hasValue(d.engine_load);
    bool has_cool = CarData::hasValue(d.coolant_temp);
    bool has_vbat = CarData::hasValue(d.battery_voltage);

    char buf[48];

    // ── 点火角 + 指针 + 极值 ──
    if (has_tim) {
        float tim = d.timing_advance;

        bool minmax_changed = false;
        if (std::isnan(session_min_) || tim < session_min_) {
            session_min_ = tim;
            minmax_changed = true;
        }
        if (std::isnan(session_max_) || tim > session_max_) {
            session_max_ = tim;
            minmax_changed = true;
        }

        float clamped = fmaxf(SCALE_MIN, fminf(SCALE_MAX, tim));
        float target_pct = valToPct(clamped);
        needle_pct_smooth_ = EMA_ALPHA * target_pct
                           + (1.0f - EMA_ALPHA) * needle_pct_smooth_;
        if (fabsf(needle_pct_smooth_ - target_pct) < 0.1f)
            needle_pct_smooth_ = target_pct;
        updateNeedle(needle_pct_smooth_);

        if (minmax_changed) {
            updateSweepArc(session_min_, session_max_);
            snprintf(buf, sizeof(buf), "MIN %+.1f\xC2\xB0 / MAX %+.1f\xC2\xB0",
                     session_min_, session_max_);
            if (strcmp(buf, last_minmax_str_) != 0) {
                strncpy(last_minmax_str_, buf, sizeof(last_minmax_str_));
                lv_label_set_text(minmax_label_, buf);
            }
        }

        snprintf(buf, sizeof(buf), "%+.1f\xC2\xB0", tim);
        if (strcmp(buf, last_value_str_) != 0) {
            strncpy(last_value_str_, buf, sizeof(last_value_str_));
            lv_label_set_text(value_label_, buf);
            lv_obj_set_style_text_color(value_label_, cValueOn(), 0);
        }
    } else {
        float zero_pct = valToPct(0.0f);
        needle_pct_smooth_ = EMA_ALPHA * zero_pct
                           + (1.0f - EMA_ALPHA) * needle_pct_smooth_;
        if (fabsf(needle_pct_smooth_ - zero_pct) < 0.1f)
            needle_pct_smooth_ = zero_pct;
        updateNeedle(needle_pct_smooth_);

        if (strcmp(last_value_str_, "---") != 0) {
            strcpy(last_value_str_, "---");
            lv_label_set_text(value_label_, "---");
            lv_obj_set_style_text_color(value_label_, cValueOff(), 0);
        }
        if (last_minmax_str_[0] != '\0') {
            last_minmax_str_[0] = '\0';
            lv_label_set_text(minmax_label_, "MIN --- / MAX ---");
        }
    }

    // ── 引擎负载 ──
    if (bar_ && has_load) {
        int16_t bar_val = (int16_t)fminf(d.engine_load, 100.0f);
        if (bar_val != last_bar_val_) {
            last_bar_val_ = bar_val;
            lv_bar_set_value(bar_, bar_val, LV_ANIM_OFF);
            float load = d.engine_load;
            lv_color_t c = (load < 50.0f)  ? lv_color_make(0, 180, 255) :
                           (load < 75.0f)  ? lv_color_make(0, 200, 100) :
                           (load < 90.0f)  ? lv_color_make(255, 165, 0) :
                                             lv_color_make(255, 50, 50);
            lv_obj_set_style_bg_color(bar_, c, LV_PART_INDICATOR);
        }
    }

    // ── 水温 ──
    if (coolant_label_) {
        char tmp[16];
        if (has_cool)
            snprintf(tmp, sizeof(tmp), "%.0f\xC2\xB0""C", d.coolant_temp);
        else
            snprintf(tmp, sizeof(tmp), "--");
        if (strcmp(tmp, last_coolant_str_) != 0) {
            strncpy(last_coolant_str_, tmp, sizeof(last_coolant_str_));
            lv_label_set_text(coolant_label_, tmp);
            lv_color_t c;
            if (!has_cool)                       c = lv_color_make(100, 100, 100);
            else if (d.coolant_temp < 60.0f)     c = lv_color_make(0, 150, 255);
            else if (d.coolant_temp < 95.0f)     c = lv_color_make(0, 200, 100);
            else                                 c = lv_color_make(255, 100, 100);
            lv_obj_set_style_text_color(coolant_label_, c, 0);
        }
    }

    // ── 电压 ──
    if (vbat_label_) {
        char tmp[16];
        if (has_vbat)
            snprintf(tmp, sizeof(tmp), "%.1fV", d.battery_voltage);
        else
            snprintf(tmp, sizeof(tmp), "--");
        if (strcmp(tmp, last_vbat_str_) != 0) {
            strncpy(last_vbat_str_, tmp, sizeof(last_vbat_str_));
            lv_label_set_text(vbat_label_, tmp);
            lv_color_t c;
            if (!has_vbat)                           c = lv_color_make(100, 100, 100);
            else if (d.battery_voltage < 12.0f)      c = lv_color_make(255, 100, 100);
            else if (d.battery_voltage < 13.5f)      c = lv_color_make(255, 180, 0);
            else                                     c = lv_color_make(0, 200, 100);
            lv_obj_set_style_text_color(vbat_label_, c, 0);
        }
    }
}

// ============ 长按重置极值 ============
bool TimingCard::onLongPress(int16_t local_x, int16_t local_y) {
    (void)local_x; (void)local_y;
    resetPeaks();
    return true;
}

void TimingCard::resetPeaks() {
    session_min_ = NAN;
    session_max_ = NAN;
    last_minmax_str_[0] = '\0';
    if (sweep_arc_) lv_arc_set_angles(sweep_arc_, 0, 0);
    if (minmax_label_) lv_label_set_text(minmax_label_, "MIN --- / MAX ---");
    LOG_INFO("[TimingCard] 峰值已清零");
}

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
