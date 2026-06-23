#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "digital_card.h"
#include "../screen_profile.h"
#include "types/globals.h"
#include "types/car_data.h"
#include "utils/debug.h"

#include <cstdio>
#include <cmath>
#include <cstring>

// 自定义数字字体（在 src/ui/led/lvgl/fonts/ 下，由平台 lv_conf 链接）
LV_FONT_DECLARE(lv_font_digital_96)
LV_FONT_DECLARE(lv_font_digital_80)
LV_FONT_DECLARE(lv_font_digital_64)
LV_FONT_DECLARE(lv_font_unit_32)

namespace UI {

// ============ 颜色 ============
static inline lv_color_t cBar()   { return lv_color_make(230, 0, 18); }  // Nismo 红
static inline lv_color_t cWhite() { return lv_color_white(); }

// ============ 辅助 ============
static inline int16_t boundsW(const lv_area_t& b) { return b.x2 - b.x1 + 1; }
static inline int16_t boundsH(const lv_area_t& b) { return b.y2 - b.y1 + 1; }
static inline int16_t shortSide(const lv_area_t& b) {
    int16_t w = boundsW(b), h = boundsH(b);
    return w < h ? w : h;
}

DigitalCard::DigitalCard() = default;

DigitalCard::~DigitalCard() {
    destroyObjects();
}

// ============ 弦计算（CIRCLE 贴圆边 / RECT 普通 padding） ============
int16_t DigitalCard::chordHalfAt(int16_t y_local) const {
    if (currentProfile().shape != ShapeKind::CIRCLE) {
        return boundsW(bounds_) / 2;
    }
    int16_t h = boundsH(bounds_);
    int16_t r = shortSide(bounds_) / 2;
    int16_t dy = y_local - h / 2;
    if (dy < 0) dy = -dy;
    if (dy >= r) return 0;
    return (int16_t)sqrtf((float)r * r - (float)(dy * dy));
}

// ============ 几何 + 字号 ============
void DigitalCard::selectGeometryAndFonts() {
    int16_t w = boundsW(bounds_);
    int16_t h = boundsH(bounds_);
    int16_t ss = shortSide(bounds_);

    // 几何按比例换算（旧 480 基准）
    bar_h_         = (int16_t)(h *  50L / 480);
    val_h_         = (int16_t)(h *  70L / 480);
    section_pitch_ = bar_h_ + val_h_;
    // 红条横跨整个 container（圆屏上视觉等同旧 BAR_W=600 超出 480 的效果）
    bar_w_         = w;
    title_inset_   = (int16_t)(w *  25L / 480);
    unit_inset_    = (int16_t)(w *  25L / 480);
    afr_val_gap_   = (int16_t)(w *  20L / 480);
    afr_unit_gap_  = (int16_t)(w *   8L / 480);

    // 字号档位（自定义数字字体优先，小卡片回退 Montserrat）
    const ScreenProfile& p = currentProfile();
    if (ss >= 400) {
        font_value_main_ = &lv_font_digital_96;
        font_value_bat_  = &lv_font_digital_80;
        font_value_afr2_ = &lv_font_digital_64;
        font_unit_       = &lv_font_unit_32;
        font_title_      = p.font_xl;        // 32px 标题
    } else if (ss >= 300) {
        font_value_main_ = &lv_font_digital_80;
        font_value_bat_  = &lv_font_digital_64;
        font_value_afr2_ = &lv_font_digital_64;
        font_unit_       = &lv_font_unit_32;
        font_title_      = p.font_lg;        // 24px
    } else {
        font_value_main_ = p.font_2xl;       // Montserrat 48px fallback
        font_value_bat_  = p.font_xl;        // 32px
        font_value_afr2_ = p.font_lg;        // 24px
        font_unit_       = p.font_md;        // 20px
        font_title_      = p.font_md;
    }
}

// ============ 共用：横杠 + 标题 ============
void DigitalCard::makeBarAndTitle(lv_obj_t* parent, const char* title_text,
                                  int16_t bar_center_y) {
    int16_t h = boundsH(bounds_);

    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, bar_w_, bar_h_);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, bar_center_y - h / 2);
    lv_obj_set_style_bg_color(bar, cBar(), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_FLAGS(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

    int16_t bar_chord_half = chordHalfAt(bar_center_y);
    // 标题贴弦内侧（圆屏：贴圆边；矩形屏：贴边 + padding）
    int16_t title_x_from_center = -bar_chord_half + title_inset_;

    lv_obj_t* title = lv_label_create(parent);
    lv_obj_set_style_text_font(title, font_title_, 0);
    lv_obj_set_style_text_color(title, cWhite(), 0);
    lv_label_set_text(title, title_text);
    lv_obj_align(title, LV_ALIGN_LEFT_MID,
                 boundsW(bounds_) / 2 + title_x_from_center,
                 bar_center_y - h / 2);
}

// ============ 标准行：值居中 + 单位右弧 ============
void DigitalCard::makeSection(lv_obj_t* parent, Section& s, const char* title_text,
                              int16_t bar_center_y,
                              const lv_font_t* val_font, bool has_unit) {
    makeBarAndTitle(parent, title_text, bar_center_y);

    int16_t h = boundsH(bounds_);
    int16_t val_center_y = bar_center_y + bar_h_ / 2 + val_h_ / 2;

    s.value = lv_label_create(parent);
    lv_obj_set_style_text_font(s.value, val_font, 0);
    lv_obj_set_style_text_color(s.value, cWhite(), 0);
    lv_obj_set_style_text_align(s.value, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s.value, "--");
    lv_obj_align(s.value, LV_ALIGN_CENTER, 0, val_center_y - h / 2);

    if (has_unit) {
        int16_t unit_chord_half = chordHalfAt(val_center_y);
        int16_t unit_x_from_center = unit_chord_half - unit_inset_;

        s.unit = lv_label_create(parent);
        lv_obj_set_style_text_font(s.unit, font_unit_, 0);
        lv_obj_set_style_text_color(s.unit, cWhite(), 0);
        lv_label_set_text(s.unit, "--");
        lv_obj_align(s.unit, LV_ALIGN_RIGHT_MID,
                     unit_x_from_center - boundsW(bounds_) / 2,
                     val_center_y - h / 2);
    }
}

// ============ AFR 行：val 居中 ── val2 ── %（链式 align_to） ============
void DigitalCard::makeAfrSection(lv_obj_t* parent, Section& s, int16_t bar_center_y) {
    makeBarAndTitle(parent, "AFR", bar_center_y);

    int16_t h = boundsH(bounds_);
    int16_t val_center_y = bar_center_y + bar_h_ / 2 + val_h_ / 2;

    // 主数值 (大字)
    s.value = lv_label_create(parent);
    lv_obj_set_style_text_font(s.value, font_value_main_, 0);
    lv_obj_set_style_text_color(s.value, cWhite(), 0);
    lv_obj_set_style_text_align(s.value, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(s.value, "--.-");
    lv_obj_align(s.value, LV_ALIGN_CENTER, 0, val_center_y - h / 2);

    // 副数 (中字，锚 value 右)
    s.value2 = lv_label_create(parent);
    lv_obj_set_style_text_font(s.value2, font_value_afr2_, 0);
    lv_obj_set_style_text_color(s.value2, cWhite(), 0);
    lv_label_set_text(s.value2, "--");
    lv_obj_align_to(s.value2, s.value, LV_ALIGN_OUT_RIGHT_MID, afr_val_gap_, 0);

    // 单位 % (锚 value2 右)
    s.unit = lv_label_create(parent);
    lv_obj_set_style_text_font(s.unit, font_unit_, 0);
    lv_obj_set_style_text_color(s.unit, cWhite(), 0);
    lv_label_set_text(s.unit, "%");
    lv_obj_align_to(s.unit, s.value2, LV_ALIGN_OUT_RIGHT_MID, afr_unit_gap_, 0);
}

// ============ 创建对象 ============
void DigitalCard::createObjects(lv_obj_t* parent) {
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

    int16_t y0 = bar_h_ / 2;
    makeSection   (container_, clt_, "COOLANT", y0 + 0 * section_pitch_,
                   font_value_main_, true);
    makeSection   (container_, iat_, "INTAKE",  y0 + 1 * section_pitch_,
                   font_value_main_, true);
    makeAfrSection(container_, afr_,            y0 + 2 * section_pitch_);
    makeSection   (container_, bat_, "BATTERY", y0 + 3 * section_pitch_,
                   font_value_bat_, true);

    // 静态单位文字
    if (clt_.unit) lv_label_set_text(clt_.unit, "\xC2\xB0""C");
    if (iat_.unit) lv_label_set_text(iat_.unit, "\xC2\xB0""C");
    if (bat_.unit) lv_label_set_text(bat_.unit, "V");
    // AFR % 已在 makeAfrSection 设好
}

void DigitalCard::destroyObjects() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    clt_ = iat_ = afr_ = bat_ = {};
}

// ============ 生命周期 ============
void DigitalCard::onMount(lv_obj_t* parent, const lv_area_t& bounds) {
    bool needs_rebuild = !container_ ||
                         bounds.x1 != bounds_.x1 || bounds.y1 != bounds_.y1 ||
                         bounds.x2 != bounds_.x2 || bounds.y2 != bounds_.y2;
    bounds_ = bounds;

    if (needs_rebuild) {
        destroyObjects();
        createObjects(parent);
    } else {
        lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }
}

void DigitalCard::onUnmount() {
    if (container_) {
        lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }
}

// ============ 数据格式化辅助 ============
static void setIntFloat(lv_obj_t* lbl, float v, bool valid) {
    if (!valid) { lv_label_set_text(lbl, "--"); return; }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(v));
    lv_label_set_text(lbl, buf);
}

static void setOneDecimal(lv_obj_t* lbl, float v, bool valid) {
    if (!valid) { lv_label_set_text(lbl, "--.-"); return; }
    char buf[8];
    snprintf(buf, sizeof(buf), "%.1f", v);
    lv_label_set_text(lbl, buf);
}

// AFR 行更新（含链式 align_to 重对齐）
static void setAfr(DigitalCard::Section& s, float lambda, bool valid,
                   int16_t afr_val_gap, int16_t afr_unit_gap) {
    if (!valid) {
        lv_label_set_text(s.value,  "--.-");
        lv_label_set_text(s.value2, "--");
        return;
    }
    char buf[12];
    snprintf(buf, sizeof(buf), "%.1f", lambda * 14.7f);
    lv_label_set_text(s.value, buf);
    snprintf(buf, sizeof(buf), "%d", (int)lroundf(lambda * 100.0f));
    lv_label_set_text(s.value2, buf);
    lv_obj_update_layout(s.value);
    lv_obj_align_to(s.value2, s.value, LV_ALIGN_OUT_RIGHT_MID, afr_val_gap, 0);
    lv_obj_update_layout(s.value2);
    lv_obj_align_to(s.unit, s.value2, LV_ALIGN_OUT_RIGHT_MID, afr_unit_gap, 0);
}

// ============ 数据更新 ============
void DigitalCard::update() {
    if (!container_) return;
    const CarData& d = latestData;

    setIntFloat  (clt_.value, d.coolant_temp,    CarData::hasValue(d.coolant_temp));
    setIntFloat  (iat_.value, d.intake_temp,     CarData::hasValue(d.intake_temp));
    setAfr       (afr_,       d.air_fuel_ratio,  CarData::hasValue(d.air_fuel_ratio),
                  afr_val_gap_, afr_unit_gap_);
    setOneDecimal(bat_.value, d.battery_voltage, CarData::hasValue(d.battery_voltage));
}

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
