// v0.8.0+：LED 圆屏已迁移到 PngGaugeCard，本卡片暂保留供 P4 后续接入
#if defined(DISPLAY_TYPE_P4_BAR)

#include "gauge_meter_card.h"
#include "../screen_profile.h"
#include "../../led/gauge_params.h"
#include <raceguard/data.h>
#include <raceguard/car_data.h>
#include <raceguard/log.h>

#if defined(DISPLAY_TYPE_P4_BAR)
// P4 (ESP-IDF) — 用 esp_timer / nvs_flash 替代 Arduino API
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
static inline uint32_t card_millis() {
    return (uint32_t)(esp_timer_get_time() / 1000);
}
#else
// 圆屏 (Arduino) — 原 Arduino API + Preferences (NVS 封装)
#include <Arduino.h>
#include <Preferences.h>
static inline uint32_t card_millis() { return millis(); }
#endif

#include <cstdio>

namespace UI {

static constexpr float PI_F        = 3.14159265f;
static constexpr float ARC_START_DEG = 120.0f;     // 主弧起始
static constexpr float ARC_SPAN_DEG  = 240.0f;     // 主弧跨度
static constexpr float SCALE_START_DEG = 135.0f;   // 刻度/指针起始
static constexpr float SCALE_SPAN_DEG  = 210.0f;   // 刻度/指针跨度

static constexpr const char* PREFS_NS  = "led_gauge";
static constexpr const char* PREFS_KEY = "param";
static constexpr uint32_t NVS_DEFER_MS = 2000;

// ============ 辅助 ============
static inline int16_t boundsW(const lv_area_t& b) { return b.x2 - b.x1 + 1; }
static inline int16_t boundsH(const lv_area_t& b) { return b.y2 - b.y1 + 1; }
static inline int16_t shortSide(const lv_area_t& b) {
    int16_t w = boundsW(b), h = boundsH(b);
    return w < h ? w : h;
}

GaugeMeterCard::GaugeMeterCard() = default;
GaugeMeterCard::~GaugeMeterCard() { destroyObjects(); }

int GaugeMeterCard::firstEnabled(int start) {
    for (int i = 0; i < (int)GaugeParamID::COUNT; i++) {
        int idx = (start + i) % (int)GaugeParamID::COUNT;
        if (kGaugeDefs[idx].enabled) return idx;
    }
    return (int)GaugeParamID::COOLANT_TEMP;
}

int GaugeMeterCard::loadParamFromNVS() {
#if defined(DISPLAY_TYPE_P4_BAR)
    // P4: ESP-IDF nvs_flash API (要求 main_p4 已调过 nvs_flash_init)
    nvs_handle_t h;
    if (nvs_open(PREFS_NS, NVS_READONLY, &h) == ESP_OK) {
        uint8_t v = (uint8_t)GaugeParamID::COOLANT_TEMP;
        esp_err_t err = nvs_get_u8(h, PREFS_KEY, &v);
        nvs_close(h);
        if (err == ESP_OK && v < (uint8_t)GaugeParamID::COUNT) {
            if (kGaugeDefs[v].enabled) return v;
            return firstEnabled(v);
        }
    }
    return firstEnabled(0);
#else
    Preferences prefs;
    if (prefs.begin(PREFS_NS, true)) {
        uint8_t v = prefs.getUChar(PREFS_KEY, (uint8_t)GaugeParamID::COOLANT_TEMP);
        prefs.end();
        if (v < (uint8_t)GaugeParamID::COUNT) {
            if (kGaugeDefs[v].enabled) return v;
            return firstEnabled(v);
        }
    }
    return firstEnabled(0);
#endif
}

void GaugeMeterCard::scheduleNVSWrite() {
    nvs_dirty_ = true;
    nvs_dirty_time_ = card_millis();
}

void GaugeMeterCard::flushNVSIfDirty() {
    if (nvs_dirty_ && (card_millis() - nvs_dirty_time_ >= NVS_DEFER_MS)) {
#if defined(DISPLAY_TYPE_P4_BAR)
        nvs_handle_t h;
        if (nvs_open(PREFS_NS, NVS_READWRITE, &h) == ESP_OK) {
            nvs_set_u8(h, PREFS_KEY, (uint8_t)param_id_);
            nvs_commit(h);
            nvs_close(h);
        }
#else
        Preferences prefs;
        if (prefs.begin(PREFS_NS, false)) {
            prefs.putUChar(PREFS_KEY, (uint8_t)param_id_);
            prefs.end();
        }
#endif
        nvs_dirty_ = false;
    }
}

// ============ 几何 + 字号 ============
void GaugeMeterCard::selectGeometryAndFonts() {
    int16_t w = boundsW(bounds_), h = boundsH(bounds_);
    int16_t ss = shortSide(bounds_);
    // cx_/cy_ 用 container 局部坐标 (圆屏 bounds=(0,0,...) 加不加等价,
    // P4 多卡布局必须不加, 否则子节点画到 container 外被裁; 详见 timing_card 注释)
    cx_ = w / 2;
    cy_ = h / 2;
    r_outer_ = ss / 2;

    // 半径基准 = ss/2，旧实现以 240 为基准
    auto scaleF = [ss](float src) -> float { return ss * src / 480.0f; };
    arc_r_         = scaleF(232.0f);
    arc_w_         = scaleF(160.0f);
    tick_outer_r_  = scaleF(236.0f);
    tick_major_len_= scaleF(42.0f);
    tick_half_len_ = scaleF(29.0f);
    tick_small_len_= scaleF(17.0f);
    label_r_       = scaleF(155.0f);
    needle_tip_r_  = scaleF(220.0f);
    needle_tail_r_ = scaleF(28.0f);

    auto scaleI = [ss](int src) -> int16_t { return (int16_t)(ss * src / 480L); };
    hub_outer_size_= scaleI(32);
    hub_inner_size_= scaleI(20);
    lcd_val_off_x_ = scaleI(20);
    lcd_val_off_y_ = scaleI(135);
    lcd_unit_x_    = scaleI(285);
    lcd_unit_y_    = scaleI(288);
    lcd_mm_off_x_  = scaleI(5);
    lcd_mm_off_y_  = scaleI(180);
    lcd_title_x_   = scaleI(285);
    lcd_title_y_   = scaleI(240);
    lcd_title_w_   = scaleI(195);
    lcd_title_h_   = scaleI(46);
    tick_unit_x_   = scaleI(170);
    tick_unit_y_   = scaleI(123);

    if (hub_outer_size_ < 16) hub_outer_size_ = 16;
    if (hub_inner_size_ < 10) hub_inner_size_ = 10;

    const ScreenProfile& p = currentProfile();
    if (ss >= 400) {
        font_value_      = p.font_2xl;     // 48px
        font_unit_       = p.font_lg;      // 24-28px
        font_title_      = p.font_lg;
        font_minmax_     = p.font_sm;
        font_tick_label_ = p.font_xl;      // 32
        font_tick_unit_  = p.font_md;
    } else if (ss >= 300) {
        font_value_      = p.font_xl;
        font_unit_       = p.font_md;
        font_title_      = p.font_md;
        font_minmax_     = p.font_xs;
        font_tick_label_ = p.font_lg;
        font_tick_unit_  = p.font_sm;
    } else {
        font_value_      = p.font_lg;
        font_unit_       = p.font_sm;
        font_title_      = p.font_sm;
        font_minmax_     = p.font_xs;
        font_tick_label_ = p.font_md;
        font_tick_unit_  = p.font_xs;
    }
}

void GaugeMeterCard::arcPoint(float deg, float r, lv_coord_t& ox, lv_coord_t& oy) const {
    float rad = deg * PI_F / 180.0f;
    ox = (lv_coord_t)(cx_ + r * cosf(rad));
    oy = (lv_coord_t)(cy_ + r * sinf(rad));
}

void GaugeMeterCard::updateNeedle(float pct) {
    float deg = SCALE_START_DEG + pct / 100.0f * SCALE_SPAN_DEG;
    float rad = deg * PI_F / 180.0f;
    float cs = cosf(rad), sn = sinf(rad);

    float tip_x  = cx_ + needle_tip_r_  * cs;
    float tip_y  = cy_ + needle_tip_r_  * sn;
    float tail_x = cx_ - needle_tail_r_ * cs;
    float tail_y = cy_ - needle_tail_r_ * sn;

    float pad = 8.0f;
    float min_x = fminf(tip_x, tail_x) - pad;
    float min_y = fminf(tip_y, tail_y) - pad;
    lv_coord_t ox = (lv_coord_t)min_x;
    lv_coord_t oy = (lv_coord_t)min_y;

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

static void checkThreshold(const GaugeParamDef& def, float val, bool& isCrit, bool& isWarn) {
    isCrit = (!std::isnan(def.critHi) && val >= def.critHi) ||
             (!std::isnan(def.critLo) && val <= def.critLo);
    isWarn = (!isCrit) && (
             (!std::isnan(def.warnHi) && val >= def.warnHi) ||
             (!std::isnan(def.warnLo) && val <= def.warnLo));
}

void GaugeMeterCard::updateTickColors() {
    const GaugeParamDef& def = getGaugeDef((GaugeParamID)param_id_);

    for (int i = 0; i < TOTAL_TICKS; i++) {
        float pct = (float)i / (MAJOR_DIVS * TICKS_PER_DIV);
        float val = def.minVal + (def.maxVal - def.minVal) * pct;
        bool isCrit, isWarn;
        checkThreshold(def, val, isCrit, isWarn);
        bool isMajor = (i % TICKS_PER_DIV == 0);
        bool isHalf  = (i % TICKS_PER_DIV == TICKS_PER_DIV / 2);
        lv_color_t c = isCrit ? lv_color_make(220, 20, 20) :
                       isWarn ? lv_color_make(220, 130, 0) :
                       isMajor ? lv_color_make(210, 210, 210) :
                       isHalf  ? lv_color_make(140, 140, 140) :
                                 lv_color_make(90, 90, 90);
        if (tick_lines_[i]) lv_obj_set_style_line_color(tick_lines_[i], c, 0);
    }
    for (int gi = 0; gi < GLOW_TICK_COUNT; gi++) {
        int i = glow_to_tick_[gi];
        float pct = (float)i / (MAJOR_DIVS * TICKS_PER_DIV);
        float val = def.minVal + (def.maxVal - def.minVal) * pct;
        bool isCrit, isWarn;
        checkThreshold(def, val, isCrit, isWarn);
        bool isMajor = (i % TICKS_PER_DIV == 0);
        lv_color_t c = isCrit ? lv_color_make(180, 0, 0) :
                       isWarn ? lv_color_make(180, 100, 0) :
                       isMajor ? lv_color_make(160, 160, 160) :
                                 lv_color_make(100, 100, 100);
        if (tick_glows_[gi]) lv_obj_set_style_line_color(tick_glows_[gi], c, 0);
    }
    for (int li = 0; li < LABEL_COUNT; li++) {
        int mi = li * LABEL_EVERY_N;
        float pct = (float)mi / MAJOR_DIVS;
        float val = def.minVal + (def.maxVal - def.minVal) * pct;
        bool isCrit, isWarn;
        checkThreshold(def, val, isCrit, isWarn);
        lv_color_t c = isCrit ? lv_color_make(230, 40, 40) :
                       isWarn ? lv_color_make(220, 140, 0) :
                                lv_color_make(200, 200, 200);
        if (tick_labels_[li]) lv_obj_set_style_text_color(tick_labels_[li], c, 0);
    }
}

void GaugeMeterCard::syncLabels() {
    const GaugeParamDef& def = getGaugeDef((GaugeParamID)param_id_);
    if (unit_label_) lv_label_set_text(unit_label_, def.unit);
    if (title_label_) lv_label_set_text(title_label_, def.nameEN);
    if (tick_unit_label_) lv_label_set_text(tick_unit_label_, def.tickUnit);

    char buf[16];
    for (int li = 0; li < LABEL_COUNT; li++) {
        int mi = li * LABEL_EVERY_N;
        float val = def.minVal + (def.maxVal - def.minVal) * (float)mi / MAJOR_DIVS;
        if (def.tickDivisor > 1) {
            int tv = (int)(val / def.tickDivisor + 0.5f);
            snprintf(buf, sizeof(buf), "%d", tv);
        } else {
            snprintf(buf, sizeof(buf), "%.0f", val);
        }
        if (tick_labels_[li]) lv_label_set_text(tick_labels_[li], buf);
    }
    updateTickColors();
}

// ============ 创建对象 ============
void GaugeMeterCard::createObjects(lv_obj_t* parent) {
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

    // ── 主弧（深灰底板）──
    arc_ = lv_arc_create(container_);
    lv_obj_set_size(arc_, (lv_coord_t)(arc_r_ * 2), (lv_coord_t)(arc_r_ * 2));
    lv_obj_align(arc_, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(arc_, (uint16_t)ARC_START_DEG);
    lv_arc_set_bg_angles(arc_, 0, (uint16_t)ARC_SPAN_DEG);
    lv_arc_set_range(arc_, 0, 100);
    lv_arc_set_value(arc_, 0);
    lv_obj_set_style_arc_color(arc_, lv_color_make(40, 40, 40), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_, (lv_coord_t)arc_w_, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc_, false, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc_, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc_, 0, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(arc_, 0, 0);
    lv_obj_clear_flag(arc_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // ── 刻度线 ──
    int glow_idx = 0;
    for (int i = 0; i < TOTAL_TICKS; i++) {
        float pct = (float)i / (MAJOR_DIVS * TICKS_PER_DIV);
        float deg = SCALE_START_DEG + pct * SCALE_SPAN_DEG;
        bool isMajor = (i % TICKS_PER_DIV == 0);
        bool isHalf  = (i % TICKS_PER_DIV == TICKS_PER_DIV / 2);
        float len = isMajor ? tick_major_len_ : (isHalf ? tick_half_len_ : tick_small_len_);

        arcPoint(deg, tick_outer_r_,       tick_pts_[i][0].x, tick_pts_[i][0].y);
        arcPoint(deg, tick_outer_r_ - len, tick_pts_[i][1].x, tick_pts_[i][1].y);

        if ((isMajor || isHalf) && glow_idx < GLOW_TICK_COUNT) {
            int glow_w = isMajor ? 15 : 10;
            tick_glows_[glow_idx] = lv_line_create(container_);
            lv_line_set_points(tick_glows_[glow_idx], tick_pts_[i], 2);
            lv_obj_set_style_line_width(tick_glows_[glow_idx], glow_w, 0);
            lv_obj_set_style_line_opa(tick_glows_[glow_idx], 60, 0);
            lv_obj_set_style_line_rounded(tick_glows_[glow_idx], false, 0);
            lv_obj_clear_flag(tick_glows_[glow_idx], LV_OBJ_FLAG_CLICKABLE);
            glow_to_tick_[glow_idx] = i;
            glow_idx++;
        }
        int main_w = isMajor ? 5 : (isHalf ? 3 : 2);
        tick_lines_[i] = lv_line_create(container_);
        lv_line_set_points(tick_lines_[i], tick_pts_[i], 2);
        lv_obj_set_style_line_width(tick_lines_[i], main_w, 0);
        lv_obj_set_style_line_rounded(tick_lines_[i], false, 0);
        lv_obj_clear_flag(tick_lines_[i], LV_OBJ_FLAG_CLICKABLE);
    }

    // ── 刻度标签 ──
    int16_t label_box = (int16_t)(shortSide(bounds_) * 68L / 480);
    int16_t label_half = label_box / 2;
    int16_t label_h_half = (int16_t)(shortSide(bounds_) * 17L / 480);
    for (int li = 0; li < LABEL_COUNT; li++) {
        int mi = li * LABEL_EVERY_N;
        float pct = (float)mi / MAJOR_DIVS;
        float deg = SCALE_START_DEG + pct * SCALE_SPAN_DEG;
        lv_coord_t lx, ly;
        arcPoint(deg, label_r_, lx, ly);

        tick_labels_[li] = lv_label_create(container_);
        lv_obj_set_style_text_font(tick_labels_[li], font_tick_label_, 0);
        lv_obj_set_style_text_align(tick_labels_[li], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(tick_labels_[li], label_box);
        lv_obj_set_pos(tick_labels_[li], lx - label_half, ly - label_h_half);
        lv_obj_clear_flag(tick_labels_[li], LV_OBJ_FLAG_CLICKABLE);
    }

    // ── 三层指针 ──
    int16_t needle_w_glow = (int16_t)(shortSide(bounds_) * 15L / 480);
    int16_t needle_w_body = (int16_t)(shortSide(bounds_) * 10L / 480);
    int16_t needle_w_core = (int16_t)(shortSide(bounds_) * 4L / 480);
    if (needle_w_glow < 5) needle_w_glow = 5;
    if (needle_w_body < 3) needle_w_body = 3;
    if (needle_w_core < 2) needle_w_core = 2;

    needle_pts_[0] = needle_pts_[1] = {cx_, cy_};
    needle_glow_ = lv_line_create(container_);
    lv_line_set_points(needle_glow_, needle_pts_, 2);
    lv_obj_set_style_line_color(needle_glow_, lv_color_make(180, 0, 0), 0);
    lv_obj_set_style_line_width(needle_glow_, needle_w_glow, 0);
    lv_obj_set_style_line_opa(needle_glow_, 80, 0);
    lv_obj_set_style_line_rounded(needle_glow_, true, 0);
    lv_obj_clear_flag(needle_glow_, LV_OBJ_FLAG_CLICKABLE);

    needle_body_ = lv_line_create(container_);
    lv_line_set_points(needle_body_, needle_pts_, 2);
    lv_obj_set_style_line_color(needle_body_, lv_color_make(220, 0, 0), 0);
    lv_obj_set_style_line_width(needle_body_, needle_w_body, 0);
    lv_obj_set_style_line_opa(needle_body_, 180, 0);
    lv_obj_set_style_line_rounded(needle_body_, true, 0);
    lv_obj_clear_flag(needle_body_, LV_OBJ_FLAG_CLICKABLE);

    needle_core_ = lv_line_create(container_);
    lv_line_set_points(needle_core_, needle_pts_, 2);
    lv_obj_set_style_line_color(needle_core_, lv_color_make(255, 60, 40), 0);
    lv_obj_set_style_line_width(needle_core_, needle_w_core, 0);
    lv_obj_set_style_line_rounded(needle_core_, true, 0);
    lv_obj_clear_flag(needle_core_, LV_OBJ_FLAG_CLICKABLE);

    // ── 轴盖外圈 + 内圈 ──
    lv_obj_t* hub_outer = lv_obj_create(container_);
    lv_obj_set_size(hub_outer, hub_outer_size_, hub_outer_size_);
    lv_obj_align(hub_outer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(hub_outer, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(hub_outer, lv_color_make(60, 60, 60), 0);
    lv_obj_set_style_bg_opa(hub_outer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(hub_outer, lv_color_make(40, 40, 40), 0);
    lv_obj_set_style_border_width(hub_outer, 2, 0);
    lv_obj_clear_flag(hub_outer, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    needle_hub_ = lv_obj_create(container_);
    lv_obj_set_size(needle_hub_, hub_inner_size_, hub_inner_size_);
    lv_obj_align(needle_hub_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(needle_hub_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(needle_hub_, lv_color_make(25, 25, 25), 0);
    lv_obj_set_style_bg_opa(needle_hub_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(needle_hub_, lv_color_make(180, 0, 0), 0);
    lv_obj_set_style_border_width(needle_hub_, 2, 0);
    lv_obj_clear_flag(needle_hub_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // 刻度单位标签
    int16_t tu_w = (int16_t)(shortSide(bounds_) * 140L / 480);
    tick_unit_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(tick_unit_label_, font_tick_unit_, 0);
    lv_obj_set_style_text_color(tick_unit_label_, lv_color_make(220, 30, 20), 0);
    lv_obj_set_style_text_align(tick_unit_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_opa(tick_unit_label_, 200, 0);
    lv_obj_set_width(tick_unit_label_, tu_w);
    lv_obj_set_pos(tick_unit_label_, tick_unit_x_, tick_unit_y_);
    lv_obj_clear_flag(tick_unit_label_, LV_OBJ_FLAG_CLICKABLE);

    updateNeedle(0);

    // ── LCD 视窗弧 (米色) ──
    lcd_panel_ = lv_arc_create(container_);
    lv_obj_set_size(lcd_panel_, (lv_coord_t)(arc_r_ * 2), (lv_coord_t)(arc_r_ * 2));
    lv_obj_align(lcd_panel_, LV_ALIGN_CENTER, 0, 0);
    lv_arc_set_rotation(lcd_panel_, 0);
    lv_arc_set_bg_angles(lcd_panel_, 0, 120);
    lv_arc_set_range(lcd_panel_, 0, 1);
    lv_arc_set_value(lcd_panel_, 0);
    lv_obj_set_style_arc_color(lcd_panel_, lv_color_make(200, 198, 160), LV_PART_MAIN);
    lv_obj_set_style_arc_width(lcd_panel_, (lv_coord_t)arc_w_, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(lcd_panel_, false, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(lcd_panel_, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(lcd_panel_, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(lcd_panel_, 0, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(lcd_panel_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lcd_panel_, 0, 0);
    lv_obj_clear_flag(lcd_panel_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // 数值
    val_label_ = lv_label_create(container_);
    lv_label_set_text(val_label_, "---");
    lv_obj_set_style_text_font(val_label_, font_value_, 0);
    lv_obj_set_style_text_color(val_label_, lv_color_make(120, 118, 100), 0);
    lv_obj_set_style_text_align(val_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(val_label_, lv_color_make(200, 198, 160), 0);
    lv_obj_set_style_bg_opa(val_label_, LV_OPA_COVER, 0);
    lv_obj_set_width(val_label_, (lv_coord_t)arc_w_);
    lv_obj_align(val_label_, LV_ALIGN_CENTER, lcd_val_off_x_, lcd_val_off_y_);

    // 单位
    int16_t unit_w = (int16_t)(shortSide(bounds_) * 195L / 480);
    unit_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(unit_label_, font_unit_, 0);
    lv_obj_set_style_text_color(unit_label_, lv_color_make(80, 82, 70), 0);
    lv_obj_set_style_text_align(unit_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(unit_label_, unit_w);
    lv_obj_set_pos(unit_label_, lcd_unit_x_, lcd_unit_y_);

    // MIN/MAX
    mm_label_ = lv_label_create(container_);
    lv_label_set_text(mm_label_, "");
    lv_obj_set_style_text_font(mm_label_, font_minmax_, 0);
    lv_obj_set_style_text_color(mm_label_, lv_color_make(100, 100, 85), 0);
    lv_obj_set_style_text_align(mm_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(mm_label_, lv_color_make(200, 198, 160), 0);
    lv_obj_set_style_bg_opa(mm_label_, LV_OPA_COVER, 0);
    lv_obj_set_width(mm_label_, (lv_coord_t)arc_w_);
    lv_obj_align(mm_label_, LV_ALIGN_CENTER, lcd_mm_off_x_, lcd_mm_off_y_);

    // 标题长条
    title_bar_ = lv_obj_create(container_);
    lv_obj_set_size(title_bar_, lcd_title_w_, lcd_title_h_);
    lv_obj_set_pos(title_bar_, lcd_title_x_, lcd_title_y_);
    lv_obj_set_style_bg_color(title_bar_, lv_color_make(15, 15, 22), 0);
    lv_obj_set_style_bg_opa(title_bar_, 235, 0);
    lv_obj_set_style_border_width(title_bar_, 0, 0);
    lv_obj_set_style_radius(title_bar_, 3, 0);
    lv_obj_set_style_pad_all(title_bar_, 0, 0);
    lv_obj_clear_flag(title_bar_, LV_FLAGS(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

    title_label_ = lv_label_create(title_bar_);
    lv_obj_set_style_text_font(title_label_, font_title_, 0);
    lv_obj_set_style_text_color(title_label_, lv_color_white(), 0);
    lv_obj_set_style_text_align(title_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title_label_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(title_label_, LV_OBJ_FLAG_CLICKABLE);

    syncLabels();
}

void GaugeMeterCard::destroyObjects() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    arc_ = val_label_ = unit_label_ = mm_label_ = lcd_panel_ = nullptr;
    tick_unit_label_ = title_bar_ = title_label_ = nullptr;
    needle_glow_ = needle_body_ = needle_core_ = needle_hub_ = nullptr;
    for (int i = 0; i < GLOW_TICK_COUNT; i++) tick_glows_[i] = nullptr;
    for (int i = 0; i < TOTAL_TICKS; i++) tick_lines_[i] = nullptr;
    for (int i = 0; i < LABEL_COUNT; i++) tick_labels_[i] = nullptr;
}

void GaugeMeterCard::onMount(lv_obj_t* parent, const lv_area_t& bounds) {
    bool needs_rebuild = !container_ ||
                         bounds.x1 != bounds_.x1 || bounds.y1 != bounds_.y1 ||
                         bounds.x2 != bounds_.x2 || bounds.y2 != bounds_.y2;
    bounds_ = bounds;
    if (needs_rebuild) {
        destroyObjects();
        if (param_id_ == 0) param_id_ = loadParamFromNVS();
        session_min_ = session_max_ = NAN;
        needle_pct_smooth_ = 0.0f;
        createObjects(parent);
    } else {
        lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }
}

void GaugeMeterCard::onUnmount() {
    if (container_) lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
}

void GaugeMeterCard::update() {
    if (!container_) return;
    flushNVSIfDirty();

    const GaugeParamDef& def = getGaugeDef((GaugeParamID)param_id_);
    const CarData& cd = raceguard::data::latest();
    bool has = hasGaugeValue((GaugeParamID)param_id_, cd);
    char buf[40];

    if (has) {
        float val = getGaugeValue((GaugeParamID)param_id_, cd);
        float range = def.maxVal - def.minVal;

        if (std::isnan(session_min_) || val < session_min_) session_min_ = val;
        if (std::isnan(session_max_) || val > session_max_) session_max_ = val;

        float norm = (range > 0.0f) ? (val - def.minVal) / range * 100.0f : 0.0f;
        norm = fmaxf(0.0f, fminf(100.0f, norm));

        float alpha = def.emaAlpha;
        needle_pct_smooth_ = alpha * norm + (1.0f - alpha) * needle_pct_smooth_;
        if (fabsf(needle_pct_smooth_ - norm) < 0.1f) needle_pct_smooth_ = norm;
        updateNeedle(needle_pct_smooth_);

        bool crit = (!std::isnan(def.critLo) && val <= def.critLo) ||
                    (!std::isnan(def.critHi) && val >= def.critHi);
        bool warn = (!std::isnan(def.warnLo) && val <= def.warnLo) ||
                    (!std::isnan(def.warnHi) && val >= def.warnHi);

        snprintf(buf, sizeof(buf), def.fmtStr, val);
        lv_label_set_text(val_label_, buf);
        lv_obj_set_style_text_color(val_label_,
            crit ? lv_color_make(180, 0, 0) :
            warn ? lv_color_make(180, 100, 0) :
                   lv_color_make(40, 45, 40), 0);

        if (!std::isnan(session_min_) && !std::isnan(session_max_)) {
            char lo[14], hi[14];
            snprintf(lo, sizeof(lo), def.fmtStr, session_min_);
            snprintf(hi, sizeof(hi), def.fmtStr, session_max_);
            snprintf(buf, sizeof(buf), "%s ~ %s", lo, hi);
            lv_label_set_text(mm_label_, buf);
        }
    } else {
        float alpha = def.emaAlpha;
        needle_pct_smooth_ = (1.0f - alpha) * needle_pct_smooth_;
        if (needle_pct_smooth_ < 0.1f) needle_pct_smooth_ = 0.0f;
        updateNeedle(needle_pct_smooth_);
        lv_label_set_text(val_label_, "---");
        lv_obj_set_style_text_color(val_label_, lv_color_make(120, 118, 100), 0);
    }
}

bool GaugeMeterCard::onLongPress(int16_t local_x, int16_t local_y) {
    (void)local_x; (void)local_y;
    int next = (param_id_ + 1) % (int)GaugeParamID::COUNT;
    param_id_ = firstEnabled(next);
    session_min_ = session_max_ = NAN;
    needle_pct_smooth_ = 0.0f;
    if (mm_label_) lv_label_set_text(mm_label_, "");
    updateNeedle(0);
    if (val_label_) {
        lv_label_set_text(val_label_, "---");
        lv_obj_set_style_text_color(val_label_, lv_color_make(120, 118, 100), 0);
    }
    syncLabels();
    scheduleNVSWrite();
    LOG_INFO("[GaugeCard] 切换参数 → %d (%s)",
             param_id_, getGaugeDef((GaugeParamID)param_id_).nameEN);
    return true;
}

}  // namespace UI

#endif  // DISPLAY_TYPE_P4_BAR
