#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "png_gauge_card.h"
#include <raceguard/data.h>
#include <raceguard/log.h>

#include <cstdio>
#include <cmath>
#include <cstring>

namespace UI {

static constexpr float PI_F     = 3.14159265f;
static constexpr float DESIGN_W = 480.0f;  // 底图烘焙基准（已缩到 480）

static inline int16_t boundsW(const lv_area_t& b) { return b.x2 - b.x1 + 1; }
static inline int16_t boundsH(const lv_area_t& b) { return b.y2 - b.y1 + 1; }

PngGaugeCard::PngGaugeCard(const Def* defs, uint8_t count, uint8_t default_idx)
    : defs_(defs), count_(count), cur_idx_(default_idx) {
    if (cur_idx_ >= count_ || !defs_[cur_idx_].enabled_default) {
        // 默认 idx 不可用，找下一个 enabled 的
        cur_idx_ = nextEnabledIdx();
    }
}

PngGaugeCard::~PngGaugeCard() {
    destroyObjects();
}

const PngGaugeCard::Def* PngGaugeCard::currentDef() const {
    return (defs_ && cur_idx_ < count_) ? &defs_[cur_idx_] : nullptr;
}

const char* PngGaugeCard::name() const {
    const Def* d = currentDef();
    return d ? d->name : "GAUGE";
}

uint8_t PngGaugeCard::nextEnabledIdx() const {
    for (uint8_t step = 1; step <= count_; ++step) {
        uint8_t idx = (uint8_t)((cur_idx_ + step) % count_);
        if (defs_[idx].enabled_default) return idx;
    }
    return cur_idx_;  // 都未启用，停在当前
}

void PngGaugeCard::onMount(lv_obj_t* parent, const lv_area_t& bounds) {
    bool needs_rebuild = !container_ ||
                         bounds.x1 != bounds_.x1 || bounds.y1 != bounds_.y1 ||
                         bounds.x2 != bounds_.x2 || bounds.y2 != bounds_.y2;
    bounds_ = bounds;
    if (needs_rebuild) {
        destroyObjects();
        needle_smooth_ = 0.0f;
        createObjects(parent);
    } else {
        lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }
}

void PngGaugeCard::onUnmount() {
    if (container_) lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
}

void PngGaugeCard::createObjects(lv_obj_t* parent) {
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, boundsW(bounds_), boundsH(bounds_));
    lv_obj_set_pos(container_, bounds_.x1, bounds_.y1);
    lv_obj_set_style_bg_color(container_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(container_, 0, 0);
    lv_obj_set_style_pad_all(container_, 0, 0);
    lv_obj_set_style_radius(container_, 0, 0);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);

    // 中心 = 卡片中心
    cx_ = boundsW(bounds_) / 2;
    cy_ = boundsH(bounds_) / 2;

    // 按短边相对 480 缩放（圆屏 480×480 时 scale=1.0；P4 复用时按容器尺寸）
    float scale = boundsW(bounds_) / DESIGN_W;
    needle_root_r_ = (int16_t)(25.0f  * scale);   // 指针内端在中心圆球外侧，不压住底图圆球
    needle_tip_r_  = (int16_t)(208.0f * scale);   // 接近内圈环
    // 数值 label：右下角对齐到 PNG 上 "°C" 单位的左上角
    // 烘焙坐标 unit-x=1720 right-aligned，unit-y=1600，宽 162，字号 180 (高 ~180)
    // → 单位左上角 @2048 ≈ (1718, 1510)；缩到 480 → (403, 354)
    val_label_right_x_  = (int16_t)(403.0f * scale);
    val_label_bottom_y_ = (int16_t)(354.0f * scale);

    // ── 底图层 ──
    const Def* d = currentDef();
    bg_img_ = lv_image_create(container_);
    if (d) lv_image_set_src(bg_img_, d->image);
    lv_obj_align(bg_img_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(bg_img_, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));

    // ── 指针 2 层：body + core（glow 去掉，FPS 测试，可从 git 恢复） ──
    needle_pts_[0] = needle_pts_[1] = {cx_, cy_};

    int16_t w_body = (int16_t)(10.0f * scale); if (w_body < 5) w_body = 5;
    int16_t w_core = (int16_t)(3.0f  * scale); if (w_core < 2) w_core = 2;

    needle_body_ = lv_line_create(container_);
    lv_line_set_points(needle_body_, needle_pts_, 2);
    lv_obj_set_style_line_color(needle_body_, lv_color_make(255, 55, 65), 0);
    lv_obj_set_style_line_width(needle_body_, w_body, 0);
    lv_obj_set_style_line_opa(needle_body_, 230, 0);
    lv_obj_set_style_line_rounded(needle_body_, true, 0);
    lv_obj_clear_flag(needle_body_, LV_OBJ_FLAG_CLICKABLE);

    needle_core_ = lv_line_create(container_);
    lv_line_set_points(needle_core_, needle_pts_, 2);
    lv_obj_set_style_line_color(needle_core_, lv_color_make(255, 240, 230), 0);
    lv_obj_set_style_line_width(needle_core_, w_core, 0);
    lv_obj_set_style_line_rounded(needle_core_, true, 0);
    lv_obj_clear_flag(needle_core_, LV_OBJ_FLAG_CLICKABLE);

    // ── 动态数值 label（标题/单位已在 PNG 上） ──
    // 锚点：label 的右下角对齐 (val_label_right_x_, val_label_bottom_y_)
    // 文字在 label 内右对齐 + 底部对齐
    val_label_ = lv_label_create(container_);
    lv_obj_set_style_text_font(val_label_, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(val_label_, lv_color_white(), 0);
    lv_obj_set_style_text_align(val_label_, LV_TEXT_ALIGN_RIGHT, 0);
    int16_t lw = (int16_t)(160.0f * scale);   // 容纳 4 位数（"-100"等）
    int16_t lh = (int16_t)(60.0f  * scale);
    lv_obj_set_size(val_label_, lw, lh);
    lv_obj_set_pos(val_label_, val_label_right_x_ - lw, val_label_bottom_y_ - lh);
    lv_label_set_text(val_label_, "---");
    lv_obj_clear_flag(val_label_, LV_OBJ_FLAG_CLICKABLE);

    updateNeedle(0.0f);

    LOG_INFO("[PngGaugeCard] mounted: %s, bounds %d×%d, center (%d,%d), scale %.2f",
             d ? d->name : "?", boundsW(bounds_), boundsH(bounds_),
             cx_, cy_, (double)scale);
}

void PngGaugeCard::destroyObjects() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    bg_img_      = nullptr;
    needle_body_ = needle_core_ = nullptr;
    val_label_   = nullptr;
    // 重置缓存比较状态，下次 mount 必须重画
    needle_pts_[0] = needle_pts_[1] = {0, 0};
    needle_obj_x_ = needle_obj_y_ = -32768;
    last_drawn_text_[0] = '\0';
    last_drawn_invalid_ = false;
}

void PngGaugeCard::updateNeedle(float pct) {
    const Def* d = currentDef();
    if (!d) return;

    // pct 限位
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 1.0f) pct = 1.0f;

    // 钟表坐标线性插值
    float clock_deg = d->angle_start_deg + pct * (d->angle_end_deg - d->angle_start_deg);
    // 钟表 → LVGL/屏幕坐标：12 点(=0°) 对应 -y，需 -90°
    float screen_deg = clock_deg - 90.0f;
    float rad = screen_deg * PI_F / 180.0f;
    float cs = cosf(rad), sn = sinf(rad);

    float start_x = cx_ + needle_root_r_ * cs;
    float start_y = cy_ + needle_root_r_ * sn;
    float tip_x   = cx_ + needle_tip_r_  * cs;
    float tip_y   = cy_ + needle_tip_r_  * sn;

    // 指针 invalidate 最小化技巧：
    //   - 把 line 对象 set_pos 到指针 bbox 的左上角
    //   - points 用相对 line 对象的局部坐标
    //   - 这样 line 的 invalidate 区域 = 指针 bbox（不是整个 container）
    //   - 用 lv_disp_enable_invalidation 包裹 3 次 set_pos+set_points，让 invalidate 合并触发
    constexpr float PAD = 8.0f;
    float min_x = fminf(start_x, tip_x) - PAD;
    float min_y = fminf(start_y, tip_y) - PAD;
    lv_coord_t ox = (lv_coord_t)min_x;
    lv_coord_t oy = (lv_coord_t)min_y;

    LinePoint new_pts[2] = {
        {(lv_coord_t)(start_x - min_x), (lv_coord_t)(start_y - min_y)},
        {(lv_coord_t)(tip_x   - min_x), (lv_coord_t)(tip_y   - min_y)},
    };
    // 像素无变化 → 跳过整个更新
    if (new_pts[0].x == needle_pts_[0].x && new_pts[0].y == needle_pts_[0].y &&
        new_pts[1].x == needle_pts_[1].x && new_pts[1].y == needle_pts_[1].y &&
        ox == needle_obj_x_ && oy == needle_obj_y_) {
        return;
    }
    needle_pts_[0]   = new_pts[0];
    needle_pts_[1]   = new_pts[1];
    needle_obj_x_    = ox;
    needle_obj_y_    = oy;

    if (!needle_body_ || !needle_core_) return;

    lv_disp_t* disp = lv_disp_get_default();
    lv_disp_enable_invalidation(disp, false);

    lv_obj_set_pos(needle_body_, ox, oy);
    lv_obj_set_pos(needle_core_, ox, oy);
    lv_line_set_points(needle_body_, needle_pts_, 2);
    lv_line_set_points(needle_core_, needle_pts_, 2);

    lv_disp_enable_invalidation(disp, true);
}

void PngGaugeCard::update() {
    if (!container_) return;
    const Def* d = currentDef();
    if (!d) return;

    constexpr float ALPHA = 0.45f;
    char buf[16];

    const CarData& cd = raceguard::data::latest();
    if (d->has_value(cd)) {
        float v = d->getter(cd);
        float range = d->value_max - d->value_min;
        float norm = (range > 0.0f) ? (v - d->value_min) / range : 0.0f;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;

        needle_smooth_ = ALPHA * norm + (1.0f - ALPHA) * needle_smooth_;
        if (fabsf(needle_smooth_ - norm) < 0.001f) needle_smooth_ = norm;
        updateNeedle(needle_smooth_);

        snprintf(buf, sizeof(buf), d->fmt, (double)v);
        if (last_drawn_invalid_ || strcmp(last_drawn_text_, buf) != 0) {
            lv_label_set_text(val_label_, buf);
            strncpy(last_drawn_text_, buf, sizeof(last_drawn_text_) - 1);
            last_drawn_text_[sizeof(last_drawn_text_) - 1] = '\0';
        }
        if (last_drawn_invalid_) {
            lv_obj_set_style_text_color(val_label_, lv_color_white(), 0);
            last_drawn_invalid_ = false;
        }
    } else {
        // 无效数据：指针缓慢回零，数值显示 "---"
        needle_smooth_ = (1.0f - ALPHA) * needle_smooth_;
        if (needle_smooth_ < 0.005f) needle_smooth_ = 0.0f;
        updateNeedle(needle_smooth_);
        if (!last_drawn_invalid_) {
            lv_label_set_text(val_label_, "---");
            lv_obj_set_style_text_color(val_label_, lv_color_make(120, 130, 145), 0);
            last_drawn_invalid_ = true;
            last_drawn_text_[0] = '\0';
        }
    }
}

void PngGaugeCard::switchTo(uint8_t idx) {
    if (idx >= count_ || idx == cur_idx_) return;
    cur_idx_ = idx;
    const Def* d = currentDef();
    if (bg_img_ && d) {
        lv_image_set_src(bg_img_, d->image);
    }
    needle_smooth_ = 0.0f;
    updateNeedle(0.0f);
    if (val_label_) {
        lv_label_set_text(val_label_, "---");
        lv_obj_set_style_text_color(val_label_, lv_color_make(120, 130, 145), 0);
    }
    LOG_INFO("[PngGaugeCard] 切换到 %s", d ? d->name : "?");
}

bool PngGaugeCard::onLongPress(int16_t local_x, int16_t local_y) {
    (void)local_x; (void)local_y;
    switchTo(nextEnabledIdx());
    return true;
}

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
