#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "fuel_trim_card.h"
#include "../screen_profile.h"
#include "types/globals.h"
#include "types/car_data.h"
#include "utils/debug.h"

#include <cstdio>
#include <cstring>

namespace UI {

static constexpr float PI_F = 3.14159265f;
static constexpr float EMA_ALPHA = 0.65f;

// ============ 辅助 ============
static inline int16_t boundsW(const lv_area_t& b) { return b.x2 - b.x1 + 1; }
static inline int16_t boundsH(const lv_area_t& b) { return b.y2 - b.y1 + 1; }
static inline int16_t shortSide(const lv_area_t& b) {
    int16_t w = boundsW(b), h = boundsH(b);
    return w < h ? w : h;
}
static inline int16_t scale480(int16_t ss, long src) {
    return (int16_t)(ss * src / 480L);
}

// ============ 构造 / 析构 ============
FuelTrimCard::FuelTrimCard() = default;
FuelTrimCard::~FuelTrimCard() { destroyObjects(); }

// ============ 几何 + 字号 ============
void FuelTrimCard::selectGeometryAndFonts() {
    int16_t w = boundsW(bounds_), h = boundsH(bounds_);
    int16_t ss = shortSide(bounds_);
    // cx_/cy_ 用 container 局部坐标 (不加 bounds.x1/y1).
    // 子节点 lv_line/lv_obj_set_pos 都基于 container 局部坐标系,
    // 圆屏 bounds 总是 (0,0,...) 加不加等价, P4 多卡布局必须不加.
    cx_ = w / 2;
    cy_ = h / 2;
    r_outer_ = ss / 2;

    // 几何比例换算（基于旧 480 实现）
    stft_size_       = scale480(ss, 440);
    stft_width_      = scale480(ss, 42);
    ltft_size_       = scale480(ss, 336);
    ltft_width_      = scale480(ss, 8);
    lambda_size_     = scale480(ss, 300);
    lambda_tip_r_    = scale480(ss, 118);
    lambda_tail_r_   = scale480(ss, 15);
    stft_offset_     = scale480(ss, 5);
    stft_tick_r_     = scale480(ss, 222);
    bank_label_off_x_= scale480(ss, 100);
    bank_label_off_y_= scale480(ss, 15);
    bank_value_off_y_= scale480(ss, 43);
    bank_ltft_off_y_ = scale480(ss, 73);
    lambda_label_y_  = scale480(ss, 95);
    afr_label_y_     = scale480(ss, 127);
    lambda_icon_x_   = scale480(ss, -42);
    lambda_icon_y_   = scale480(ss, 95);
    afr_icon_x_      = scale480(ss, -42);
    afr_icon_y_      = scale480(ss, 127);
    hub_outer_size_  = scale480(ss, 32);
    hub_inner_size_  = scale480(ss, 20);

    if (stft_width_ < 8)  stft_width_  = 8;
    if (ltft_width_ < 3)  ltft_width_  = 3;
    if (hub_outer_size_ < 16) hub_outer_size_ = 16;
    if (hub_inner_size_ < 10) hub_inner_size_ = 10;

    // 字号档位
    const ScreenProfile& p = currentProfile();
    if (ss >= 400) {
        font_stft_val_   = p.font_lg;        // 24px
        font_ltft_val_   = p.font_xs;        // 14px
        font_lambda_val_ = p.font_xl;        // 32px (旧 28，接近)
        font_afr_val_    = p.font_md;        // 20px
        font_bank_lbl_   = p.font_xs;
        font_tick_       = p.font_sm;
    } else if (ss >= 300) {
        font_stft_val_   = p.font_md;
        font_ltft_val_   = p.font_xs;
        font_lambda_val_ = p.font_lg;
        font_afr_val_    = p.font_sm;
        font_bank_lbl_   = p.font_xs;
        font_tick_       = p.font_xs;
    } else {
        font_stft_val_   = p.font_sm;
        font_ltft_val_   = p.font_xs;
        font_lambda_val_ = p.font_md;
        font_afr_val_    = p.font_xs;
        font_bank_lbl_   = p.font_xs;
        font_tick_       = p.font_xs;
    }
}

// ============ Lambda 指针更新 ============
void FuelTrimCard::updateLambdaNeedle(float pct) {
    float deg = 180.0f + pct / 100.0f * 180.0f;
    float rad = deg * PI_F / 180.0f;
    float cs = cosf(rad), sn = sinf(rad);

    float tip_x  = cx_ + lambda_tip_r_  * cs;
    float tip_y  = cy_ + lambda_tip_r_  * sn;
    float tail_x = cx_ - lambda_tail_r_ * cs;
    float tail_y = cy_ - lambda_tail_r_ * sn;

    float pad = 8.0f;
    float min_x = fminf(tip_x, tail_x) - pad;
    float min_y = fminf(tip_y, tail_y) - pad;
    lv_coord_t ox = (lv_coord_t)min_x;
    lv_coord_t oy = (lv_coord_t)min_y;

    lambda_pts_[0] = {(lv_coord_t)(tail_x - min_x), (lv_coord_t)(tail_y - min_y)};
    lambda_pts_[1] = {(lv_coord_t)(tip_x  - min_x), (lv_coord_t)(tip_y  - min_y)};

    if (!lambda_glow_ || !lambda_body_ || !lambda_core_) return;

    lv_disp_t* d = lv_disp_get_default();
    lv_disp_enable_invalidation(d, false);

    lv_obj_set_pos(lambda_glow_, ox, oy);
    lv_obj_set_pos(lambda_body_, ox, oy);
    lv_obj_set_pos(lambda_core_, ox, oy);
    lv_line_set_points(lambda_glow_, lambda_pts_, 2);
    lv_line_set_points(lambda_body_, lambda_pts_, 2);
    lv_line_set_points(lambda_core_, lambda_pts_, 2);

    lv_disp_enable_invalidation(d, true);
}

// ============ 创建对象 ============
void FuelTrimCard::createObjects(lv_obj_t* parent) {
    selectGeometryAndFonts();

    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, boundsW(bounds_), boundsH(bounds_));
    lv_obj_set_pos(container_, bounds_.x1, bounds_.y1);
    lv_obj_set_style_bg_color(container_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_radius(container_, 0, 0);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);

    // ── 通用 arc 样式辅助 ──
    auto makeArc = [&](int16_t size, int16_t xOff, int16_t rotation,
                       lv_color_t bg_color, int16_t arc_width,
                       lv_color_t indicator_color, bool symmetrical,
                       int32_t init_value) -> lv_obj_t* {
        lv_obj_t* arc = lv_arc_create(container_);
        lv_obj_set_size(arc, size, size);
        lv_obj_align(arc, LV_ALIGN_CENTER, xOff, 0);
        lv_arc_set_rotation(arc, rotation);
        lv_arc_set_bg_angles(arc, 0, 180);
        lv_arc_set_range(arc, -25, 25);
        lv_arc_set_mode(arc, symmetrical ? LV_ARC_MODE_SYMMETRICAL : LV_ARC_MODE_NORMAL);
        lv_arc_set_value(arc, init_value);
        lv_obj_set_style_arc_color(arc, bg_color, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc, arc_width, LV_PART_MAIN);
        lv_obj_set_style_arc_color(arc, indicator_color, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(arc, arc_width, LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);
        lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
        lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);
        lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(arc, 0, 0);
        lv_obj_clear_flag(arc, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
        return arc;
    };

    // ── STFT 弧 (red, symmetrical) ──
    stft_arc_b1_ = makeArc(stft_size_, -stft_offset_, 90,
                            lv_color_make(51, 51, 51), stft_width_,
                            lv_color_make(230, 0, 18), true, 0);
    stft_arc_b2_ = makeArc(stft_size_, +stft_offset_, 270,
                            lv_color_make(51, 51, 51), stft_width_,
                            lv_color_make(230, 0, 18), true, 0);

    // ── LTFT 弧 (blue, normal mode for arbitrary start/end) ──
    ltft_arc_b1_ = makeArc(ltft_size_, -stft_offset_, 90,
                            lv_color_make(35, 35, 35), ltft_width_,
                            lv_color_make(0, 100, 255), false, -25);
    ltft_arc_b2_ = makeArc(ltft_size_, +stft_offset_, 270,
                            lv_color_make(35, 35, 35), ltft_width_,
                            lv_color_make(0, 100, 255), false, -25);

    // ── STFT 外刻度线 (11 ticks × 2 bank) ──
    {
        // LVGL 9 用 lv_point_precise_t, LVGL 8 用 lv_point_t (lv_compat.h 收敛)
        static LinePoint s_tick_pts_l[11][2];
        static LinePoint s_tick_pts_r[11][2];
        const int16_t stftTicks[] = {-25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25};
        int16_t outerR = stft_tick_r_;
        int16_t cxL = cx_ - scale480(shortSide(bounds_), 5);
        int16_t cxR = cx_ + scale480(shortSide(bounds_), 5);
        for (int i = 0; i < 11; i++) {
            float stft = (float)stftTicks[i];
            bool major = (stft == 0 || fabsf(stft) == 25.0f || fabsf(stft) == 10.0f);
            int16_t tLen = major ? scale480(shortSide(bounds_), 12)
                                 : scale480(shortSide(bounds_), 7);
            int16_t tW   = major ? 2 : 1;
            lv_color_t tCol = major ? lv_color_make(150, 150, 150)
                                    : lv_color_make(80, 80, 80);

            float angL = (180.0f + stft / 25.0f * 90.0f) * PI_F / 180.0f;
            s_tick_pts_l[i][0] = {(lv_coord_t)(cxL + outerR * cosf(angL)),
                                  (lv_coord_t)(cy_ + outerR * sinf(angL))};
            s_tick_pts_l[i][1] = {(lv_coord_t)(cxL + (outerR + tLen) * cosf(angL)),
                                  (lv_coord_t)(cy_ + (outerR + tLen) * sinf(angL))};
            lv_obj_t* tl = lv_line_create(container_);
            lv_line_set_points(tl, s_tick_pts_l[i], 2);
            lv_obj_set_style_line_color(tl, tCol, 0);
            lv_obj_set_style_line_width(tl, tW, 0);

            float angR = (-stft / 25.0f * 90.0f) * PI_F / 180.0f;
            s_tick_pts_r[i][0] = {(lv_coord_t)(cxR + outerR * cosf(angR)),
                                  (lv_coord_t)(cy_ + outerR * sinf(angR))};
            s_tick_pts_r[i][1] = {(lv_coord_t)(cxR + (outerR + tLen) * cosf(angR)),
                                  (lv_coord_t)(cy_ + (outerR + tLen) * sinf(angR))};
            lv_obj_t* tr = lv_line_create(container_);
            lv_line_set_points(tr, s_tick_pts_r[i], 2);
            lv_obj_set_style_line_color(tr, tCol, 0);
            lv_obj_set_style_line_width(tr, tW, 0);
        }
    }

    // ── Bank 标签 ──
    auto createBankLabel = [&](int16_t xOff, const char* text) {
        lv_obj_t* bt = lv_label_create(container_);
        lv_label_set_text(bt, text);
        lv_obj_set_style_text_font(bt, font_bank_lbl_, 0);
        lv_obj_set_style_text_color(bt, lv_color_make(100, 100, 100), 0);
        lv_obj_align(bt, LV_ALIGN_CENTER, xOff, bank_label_off_y_);
    };
    createBankLabel(-bank_label_off_x_, "BANK 1");
    createBankLabel(+bank_label_off_x_, "BANK 2");

    // ── Lambda 仪表 ──
    // 圆屏 LVGL 8 用 lv_meter, P4 LVGL 9 用 lv_scale + section 替代.
    // 跟 timing 不同: lambda meter 没有灰色背景轨道, 只有 ticks + 红区.
    lv_obj_t* lambda_meter;

#if LVGL_VERSION_MAJOR >= 9
    // ─── LVGL 9: lv_scale (ROUND_INNER) ───
    lambda_meter = lv_scale_create(container_);
    lv_obj_set_size(lambda_meter, lambda_size_, lambda_size_);
    lv_obj_align(lambda_meter, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(lambda_meter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lambda_meter, 0, 0);
    lv_obj_set_style_pad_all(lambda_meter, 12, 0);
    lv_obj_clear_flag(lambda_meter, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // 圆形 scale, 180° 半圆, 从 9 点钟起, 量程 85..115 (Lambda × 100)
    lv_scale_set_mode(lambda_meter, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_angle_range(lambda_meter, 180);
    lv_scale_set_rotation(lambda_meter, 180);
    lv_scale_set_range(lambda_meter, 85, 115);
    lv_scale_set_total_tick_count(lambda_meter, 31);   // 31 个总刻度
    lv_scale_set_major_tick_every(lambda_meter, 5);    // 每 5 个一个 major (即 7 个 major)
    lv_scale_set_label_show(lambda_meter, false);

    // 不画背景轨道 (跟圆屏 lv_meter 视觉对齐)
    lv_obj_set_style_arc_opa(lambda_meter, LV_OPA_TRANSP, LV_PART_MAIN);

    // tick 样式
    lv_obj_set_style_line_color(lambda_meter, lv_color_make(150, 150, 150), LV_PART_INDICATOR);
    lv_obj_set_style_length(lambda_meter, 18, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(lambda_meter, 3, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(lambda_meter, lv_color_make(60, 60, 60), LV_PART_ITEMS);
    lv_obj_set_style_length(lambda_meter, 12, LV_PART_ITEMS);
    lv_obj_set_style_line_width(lambda_meter, 2, LV_PART_ITEMS);

    // 红区 style (两 section 共用)
    static lv_style_t style_lambda_red;
    static bool style_lambda_red_inited = false;
    if (!style_lambda_red_inited) {
        lv_style_init(&style_lambda_red);
        style_lambda_red_inited = true;
    }
    lv_style_set_arc_color(&style_lambda_red, lv_color_make(180, 0, 0));
    lv_style_set_arc_width(&style_lambda_red, 10);
    lv_style_set_arc_opa(&style_lambda_red, LV_OPA_COVER);

    // 红区 section 1: 85..90 (过稀)
    lv_scale_section_t* sec_red_lo = lv_scale_add_section(lambda_meter);
    lv_scale_set_section_range(lambda_meter, sec_red_lo, 85, 90);
    lv_scale_set_section_style_main(lambda_meter, sec_red_lo, &style_lambda_red);

    // 红区 section 2: 110..115 (过浓)
    lv_scale_section_t* sec_red_hi = lv_scale_add_section(lambda_meter);
    lv_scale_set_section_range(lambda_meter, sec_red_hi, 110, 115);
    lv_scale_set_section_style_main(lambda_meter, sec_red_hi, &style_lambda_red);

#else
    // ─── LVGL 8: 原 lv_meter (圆屏路径) ───
    lambda_meter = lv_meter_create(container_);
    lv_obj_set_size(lambda_meter, lambda_size_, lambda_size_);
    lv_obj_align(lambda_meter, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(lambda_meter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lambda_meter, 0, 0);
    lv_obj_set_style_pad_all(lambda_meter, 12, 0);
    lv_obj_clear_flag(lambda_meter, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    lv_meter_scale_t* scale = lv_meter_add_scale(lambda_meter);
    lv_meter_set_scale_ticks(lambda_meter, scale, 31, 2, 12, lv_color_make(60, 60, 60));
    lv_meter_set_scale_major_ticks(lambda_meter, scale, 5, 3, 18, lv_color_make(150, 150, 150), 18);
    lv_meter_set_scale_range(lambda_meter, scale, 85, 115, 180, 180);

    lv_meter_indicator_t* red_lo = lv_meter_add_arc(lambda_meter, scale, 10, lv_color_make(180, 0, 0), -4);
    lv_meter_set_indicator_start_value(lambda_meter, red_lo, 85);
    lv_meter_set_indicator_end_value(lambda_meter, red_lo, 90);
    lv_meter_indicator_t* red_hi = lv_meter_add_arc(lambda_meter, scale, 10, lv_color_make(180, 0, 0), -4);
    lv_meter_set_indicator_start_value(lambda_meter, red_hi, 110);
    lv_meter_set_indicator_end_value(lambda_meter, red_hi, 115);
#endif

    // ── 轴盖 (外圈 + 内圈) ──
    lv_obj_t* hub_outer = lv_obj_create(container_);
    lv_obj_set_size(hub_outer, hub_outer_size_, hub_outer_size_);
    lv_obj_align(hub_outer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(hub_outer, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub_outer, lv_color_make(60, 60, 60), 0);
    lv_obj_set_style_bg_opa(hub_outer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hub_outer, lv_color_make(40, 40, 40), 0);
    lv_obj_set_style_border_width(hub_outer, 2, 0);
    lv_obj_clear_flag(hub_outer, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // ── Lambda L / AFR A/F 静态图标 ──
    lv_obj_t* lambda_icon = lv_label_create(container_);
    lv_label_set_text(lambda_icon, "L");
    lv_obj_set_style_text_font(lambda_icon, font_bank_lbl_, 0);
    lv_obj_set_style_text_color(lambda_icon, lv_color_make(230, 0, 18), 0);
    lv_obj_align(lambda_icon, LV_ALIGN_CENTER, lambda_icon_x_, lambda_icon_y_);

    lv_obj_t* afr_icon = lv_label_create(container_);
    lv_label_set_text(afr_icon, "A/F");
    lv_obj_set_style_text_font(afr_icon, font_bank_lbl_, 0);
    lv_obj_set_style_text_color(afr_icon, lv_color_make(100, 100, 100), 0);
    lv_obj_align(afr_icon, LV_ALIGN_CENTER, afr_icon_x_, afr_icon_y_);

    // ── Lambda 三层发光指针 ──
    lambda_pts_[0] = lambda_pts_[1] = {0, 0};
    int16_t needle_w_glow = scale480(shortSide(bounds_), 15);
    int16_t needle_w_body = scale480(shortSide(bounds_), 10);
    int16_t needle_w_core = scale480(shortSide(bounds_), 4);
    if (needle_w_glow < 5) needle_w_glow = 5;
    if (needle_w_body < 3) needle_w_body = 3;
    if (needle_w_core < 2) needle_w_core = 2;

    lambda_glow_ = lv_line_create(container_);
    lv_line_set_points(lambda_glow_, lambda_pts_, 2);
    lv_obj_set_style_line_color(lambda_glow_, lv_color_make(180, 0, 0), 0);
    lv_obj_set_style_line_width(lambda_glow_, needle_w_glow, 0);
    lv_obj_set_style_line_opa(lambda_glow_, 80, 0);
    lv_obj_set_style_line_rounded(lambda_glow_, true, 0);
    lv_obj_clear_flag(lambda_glow_, LV_OBJ_FLAG_CLICKABLE);

    lambda_body_ = lv_line_create(container_);
    lv_line_set_points(lambda_body_, lambda_pts_, 2);
    lv_obj_set_style_line_color(lambda_body_, lv_color_make(220, 0, 0), 0);
    lv_obj_set_style_line_width(lambda_body_, needle_w_body, 0);
    lv_obj_set_style_line_opa(lambda_body_, 180, 0);
    lv_obj_set_style_line_rounded(lambda_body_, true, 0);
    lv_obj_clear_flag(lambda_body_, LV_OBJ_FLAG_CLICKABLE);

    lambda_core_ = lv_line_create(container_);
    lv_line_set_points(lambda_core_, lambda_pts_, 2);
    lv_obj_set_style_line_color(lambda_core_, lv_color_make(255, 60, 40), 0);
    lv_obj_set_style_line_width(lambda_core_, needle_w_core, 0);
    lv_obj_set_style_line_rounded(lambda_core_, true, 0);
    lv_obj_clear_flag(lambda_core_, LV_OBJ_FLAG_CLICKABLE);

    // ── 内侧轴盖（指针之上）──
    lambda_hub_ = lv_obj_create(container_);
    lv_obj_set_size(lambda_hub_, hub_inner_size_, hub_inner_size_);
    lv_obj_align(lambda_hub_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(lambda_hub_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(lambda_hub_, lv_color_make(25, 25, 25), 0);
    lv_obj_set_style_bg_opa(lambda_hub_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(lambda_hub_, lv_color_make(180, 0, 0), 0);
    lv_obj_set_style_border_width(lambda_hub_, 2, 0);
    lv_obj_clear_flag(lambda_hub_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    updateLambdaNeedle(lambda_pct_smooth_);

    // ── STFT / LTFT 数值标签 ──
    auto createValueLabels = [&](int16_t xOff,
                                 lv_obj_t*& stftLabel, lv_obj_t*& ltftLabel) {
        stftLabel = lv_label_create(container_);
        lv_label_set_text(stftLabel, "---");
        lv_obj_set_style_text_font(stftLabel, font_stft_val_, 0);
        lv_obj_set_style_text_color(stftLabel, lv_color_make(255, 255, 255), 0);
        lv_obj_set_width(stftLabel, scale480(shortSide(bounds_), 120));
        lv_obj_set_style_text_align(stftLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(stftLabel, LV_ALIGN_CENTER, xOff, bank_value_off_y_);

        ltftLabel = lv_label_create(container_);
        lv_label_set_text(ltftLabel, "LT --/--");
        lv_obj_set_style_text_font(ltftLabel, font_ltft_val_, 0);
        lv_obj_set_style_text_color(ltftLabel, lv_color_make(80, 80, 80), 0);
        lv_obj_set_width(ltftLabel, scale480(shortSide(bounds_), 120));
        lv_obj_set_style_text_align(ltftLabel, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(ltftLabel, LV_ALIGN_CENTER, xOff, bank_ltft_off_y_);
    };
    createValueLabels(-bank_label_off_x_, stft_b1_label_, ltft_b1_label_);
    createValueLabels(+bank_label_off_x_, stft_b2_label_, ltft_b2_label_);

    // ── Lambda / AFR 数值标签 ──
    lambda_label_ = lv_label_create(container_);
    lv_label_set_text(lambda_label_, "---");
    lv_obj_set_style_text_font(lambda_label_, font_lambda_val_, 0);
    lv_obj_set_style_text_color(lambda_label_, lv_color_make(255, 255, 255), 0);
    lv_obj_set_width(lambda_label_, scale480(shortSide(bounds_), 100));
    lv_obj_set_style_text_align(lambda_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lambda_label_, LV_ALIGN_CENTER, 0, lambda_label_y_);

    afr_label_ = lv_label_create(container_);
    lv_label_set_text(afr_label_, "---");
    lv_obj_set_style_text_font(afr_label_, font_afr_val_, 0);
    lv_obj_set_style_text_color(afr_label_, lv_color_make(200, 200, 200), 0);
    lv_obj_set_width(afr_label_, scale480(shortSide(bounds_), 80));
    lv_obj_set_style_text_align(afr_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(afr_label_, LV_ALIGN_CENTER, 0, afr_label_y_);
}

void FuelTrimCard::destroyObjects() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    stft_arc_b1_ = stft_arc_b2_ = ltft_arc_b1_ = ltft_arc_b2_ = nullptr;
    lambda_glow_ = lambda_body_ = lambda_core_ = lambda_hub_ = nullptr;
    stft_b1_label_ = stft_b2_label_ = ltft_b1_label_ = ltft_b2_label_ = nullptr;
    lambda_label_ = afr_label_ = nullptr;
}

// ============ 生命周期 ============
void FuelTrimCard::onMount(lv_obj_t* parent, const lv_area_t& bounds) {
    bool needs_rebuild = !container_ ||
                         bounds.x1 != bounds_.x1 || bounds.y1 != bounds_.y1 ||
                         bounds.x2 != bounds_.x2 || bounds.y2 != bounds_.y2;
    bounds_ = bounds;
    if (needs_rebuild) {
        destroyObjects();
        ltft_b1_min_ = ltft_b1_max_ = 0;
        ltft_b2_min_ = ltft_b2_max_ = 0;
        stft_b1_smooth_ = stft_b2_smooth_ = 0.0f;
        lambda_pct_smooth_ = 50.0f;
        last_arc_b1_val_ = last_arc_b2_val_ = 0;
        last_stft_b1_str_[0] = last_stft_b2_str_[0] = '\0';
        last_ltft_b1_str_[0] = last_ltft_b2_str_[0] = '\0';
        last_lambda_str_[0] = last_afr_str_[0] = '\0';
        last_ltft_b1_ang_[0] = last_ltft_b1_ang_[1] = 0;
        last_ltft_b2_ang_[0] = last_ltft_b2_ang_[1] = 0;
        createObjects(parent);
    } else {
        lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }
}

void FuelTrimCard::onUnmount() {
    if (container_) lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
}

// ============ 数据更新 ============
void FuelTrimCard::update() {
    if (!container_) return;
    const CarData& d = latestData;
    const lv_color_t colNoData = lv_color_make(100, 100, 100);
    char buf[24];

    bool has_stft_b1 = CarData::hasValue(d.short_fuel_trim_b1);
    bool has_stft_b2 = CarData::hasValue(d.short_fuel_trim_b2);
    bool has_ltft_b1 = CarData::hasValue(d.long_fuel_trim_b1);
    bool has_ltft_b2 = CarData::hasValue(d.long_fuel_trim_b2);
    bool has_lambda  = CarData::hasValue(d.air_fuel_ratio);

    // ── Bank 1 STFT ──
    if (has_stft_b1) {
        float v = d.short_fuel_trim_b1;
        stft_b1_smooth_ = EMA_ALPHA * v + (1.0f - EMA_ALPHA) * stft_b1_smooth_;
        if (fabsf(stft_b1_smooth_ - v) < 0.1f) stft_b1_smooth_ = v;
        if (stft_arc_b1_) {
            int16_t arc_val = (int16_t)fmaxf(-25, fminf(25, stft_b1_smooth_));
            if (arc_val != last_arc_b1_val_) {
                last_arc_b1_val_ = arc_val;
                lv_arc_set_value(stft_arc_b1_, arc_val);
            }
        }
        if (stft_b1_label_) {
            snprintf(buf, sizeof(buf), "%+.1f%%", v);
            if (strcmp(buf, last_stft_b1_str_) != 0) {
                strncpy(last_stft_b1_str_, buf, sizeof(last_stft_b1_str_));
                lv_label_set_text(stft_b1_label_, buf);
                lv_obj_set_style_text_color(stft_b1_label_, lv_color_make(255, 255, 255), 0);
            }
        }
    } else {
        stft_b1_smooth_ = (1.0f - EMA_ALPHA) * stft_b1_smooth_;
        if (fabsf(stft_b1_smooth_) < 0.1f) stft_b1_smooth_ = 0.0f;
        if (stft_arc_b1_) {
            int16_t arc_val = (int16_t)stft_b1_smooth_;
            if (arc_val != last_arc_b1_val_) {
                last_arc_b1_val_ = arc_val;
                lv_arc_set_value(stft_arc_b1_, arc_val);
            }
        }
        if (stft_b1_label_ && strcmp(last_stft_b1_str_, "---") != 0) {
            strcpy(last_stft_b1_str_, "---");
            lv_label_set_text(stft_b1_label_, "---");
            lv_obj_set_style_text_color(stft_b1_label_, colNoData, 0);
        }
    }

    // ── Bank 2 STFT ──
    if (has_stft_b2) {
        float v = d.short_fuel_trim_b2;
        stft_b2_smooth_ = EMA_ALPHA * v + (1.0f - EMA_ALPHA) * stft_b2_smooth_;
        if (fabsf(stft_b2_smooth_ - v) < 0.1f) stft_b2_smooth_ = v;
        if (stft_arc_b2_) {
            int16_t arc_val = (int16_t)fmaxf(-25, fminf(25, -stft_b2_smooth_));
            if (arc_val != last_arc_b2_val_) {
                last_arc_b2_val_ = arc_val;
                lv_arc_set_value(stft_arc_b2_, arc_val);
            }
        }
        if (stft_b2_label_) {
            snprintf(buf, sizeof(buf), "%+.1f%%", v);
            if (strcmp(buf, last_stft_b2_str_) != 0) {
                strncpy(last_stft_b2_str_, buf, sizeof(last_stft_b2_str_));
                lv_label_set_text(stft_b2_label_, buf);
                lv_obj_set_style_text_color(stft_b2_label_, lv_color_make(255, 255, 255), 0);
            }
        }
    } else {
        stft_b2_smooth_ = (1.0f - EMA_ALPHA) * stft_b2_smooth_;
        if (fabsf(stft_b2_smooth_) < 0.1f) stft_b2_smooth_ = 0.0f;
        if (stft_arc_b2_) {
            int16_t arc_val = (int16_t)(-stft_b2_smooth_);
            if (arc_val != last_arc_b2_val_) {
                last_arc_b2_val_ = arc_val;
                lv_arc_set_value(stft_arc_b2_, arc_val);
            }
        }
        if (stft_b2_label_ && strcmp(last_stft_b2_str_, "---") != 0) {
            strcpy(last_stft_b2_str_, "---");
            lv_label_set_text(stft_b2_label_, "---");
            lv_obj_set_style_text_color(stft_b2_label_, colNoData, 0);
        }
    }

    // ── LTFT 峰值追踪 + 内弧 ──
    if (has_ltft_b1) {
        float v = d.long_fuel_trim_b1;
        bool changed = false;
        if (v < ltft_b1_min_) { ltft_b1_min_ = v; changed = true; }
        if (v > ltft_b1_max_) { ltft_b1_max_ = v; changed = true; }
        if (changed && ltft_arc_b1_) {
            float cmin = fmaxf(-25, fminf(25, ltft_b1_min_));
            float cmax = fmaxf(-25, fminf(25, ltft_b1_max_));
            uint16_t angStart = (uint16_t)(90.0f + cmin / 25.0f * 90.0f);
            uint16_t angEnd   = (uint16_t)(90.0f + cmax / 25.0f * 90.0f);
            if (angStart < angEnd &&
                (angStart != last_ltft_b1_ang_[0] || angEnd != last_ltft_b1_ang_[1])) {
                last_ltft_b1_ang_[0] = angStart;
                last_ltft_b1_ang_[1] = angEnd;
                lv_arc_set_angles(ltft_arc_b1_, angStart, angEnd);
            }
        }
    }
    if (has_ltft_b2) {
        float v = d.long_fuel_trim_b2;
        bool changed = false;
        if (v < ltft_b2_min_) { ltft_b2_min_ = v; changed = true; }
        if (v > ltft_b2_max_) { ltft_b2_max_ = v; changed = true; }
        if (changed && ltft_arc_b2_) {
            float cmin = fmaxf(-25, fminf(25, ltft_b2_min_));
            float cmax = fmaxf(-25, fminf(25, ltft_b2_max_));
            uint16_t angStart = (uint16_t)(90.0f - cmax / 25.0f * 90.0f);
            uint16_t angEnd   = (uint16_t)(90.0f - cmin / 25.0f * 90.0f);
            if (angStart < angEnd &&
                (angStart != last_ltft_b2_ang_[0] || angEnd != last_ltft_b2_ang_[1])) {
                last_ltft_b2_ang_[0] = angStart;
                last_ltft_b2_ang_[1] = angEnd;
                lv_arc_set_angles(ltft_arc_b2_, angStart, angEnd);
            }
        }
    }

    // ── LTFT 文字 ──
    if (ltft_b1_label_) {
        if (has_ltft_b1)
            snprintf(buf, sizeof(buf), "LT %+.0f/%+.0f", ltft_b1_max_, ltft_b1_min_);
        else
            snprintf(buf, sizeof(buf), "LT --/--");
        if (strcmp(buf, last_ltft_b1_str_) != 0) {
            strncpy(last_ltft_b1_str_, buf, sizeof(last_ltft_b1_str_));
            lv_label_set_text(ltft_b1_label_, buf);
        }
    }
    if (ltft_b2_label_) {
        if (has_ltft_b2)
            snprintf(buf, sizeof(buf), "LT %+.0f/%+.0f", ltft_b2_max_, ltft_b2_min_);
        else
            snprintf(buf, sizeof(buf), "LT --/--");
        if (strcmp(buf, last_ltft_b2_str_) != 0) {
            strncpy(last_ltft_b2_str_, buf, sizeof(last_ltft_b2_str_));
            lv_label_set_text(ltft_b2_label_, buf);
        }
    }

    // ── Lambda + AFR ──
    if (has_lambda) {
        float lambda = d.air_fuel_ratio;
        float mval = fmaxf(85.0f, fminf(115.0f, lambda * 100.0f));
        float target_pct = (mval - 85.0f) / 30.0f * 100.0f;
        lambda_pct_smooth_ = EMA_ALPHA * target_pct
                           + (1.0f - EMA_ALPHA) * lambda_pct_smooth_;
        if (fabsf(lambda_pct_smooth_ - target_pct) < 0.1f)
            lambda_pct_smooth_ = target_pct;
        updateLambdaNeedle(lambda_pct_smooth_);

        if (lambda_label_) {
            snprintf(buf, sizeof(buf), "%.3f", lambda);
            if (strcmp(buf, last_lambda_str_) != 0) {
                strncpy(last_lambda_str_, buf, sizeof(last_lambda_str_));
                lv_label_set_text(lambda_label_, buf);
                float dev = fabsf(lambda - 1.0f);
                lv_color_t lc = dev < 0.03f ? lv_color_make(255, 255, 255) :
                                dev < 0.05f ? lv_color_make(180, 0, 14) :
                                              lv_color_make(230, 0, 18);
                lv_obj_set_style_text_color(lambda_label_, lc, 0);
            }
        }
        if (afr_label_) {
            snprintf(buf, sizeof(buf), "%.1f", lambda * 14.7f);
            if (strcmp(buf, last_afr_str_) != 0) {
                strncpy(last_afr_str_, buf, sizeof(last_afr_str_));
                lv_label_set_text(afr_label_, buf);
            }
        }
    } else {
        float center_pct = 50.0f;
        lambda_pct_smooth_ = EMA_ALPHA * center_pct
                           + (1.0f - EMA_ALPHA) * lambda_pct_smooth_;
        if (fabsf(lambda_pct_smooth_ - center_pct) < 0.1f)
            lambda_pct_smooth_ = center_pct;
        updateLambdaNeedle(lambda_pct_smooth_);

        if (lambda_label_ && strcmp(last_lambda_str_, "---") != 0) {
            strcpy(last_lambda_str_, "---");
            lv_label_set_text(lambda_label_, "---");
            lv_obj_set_style_text_color(lambda_label_, colNoData, 0);
        }
        if (afr_label_ && strcmp(last_afr_str_, "---") != 0) {
            strcpy(last_afr_str_, "---");
            lv_label_set_text(afr_label_, "---");
        }
    }
}

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
