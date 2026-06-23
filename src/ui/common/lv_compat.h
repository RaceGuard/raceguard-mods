#pragma once

/**
 * LVGL 8 / 9 跨版本兼容垫片
 *
 * 圆屏 (round-led-21) 用 lvgl@8.3.11 (LVGL_VERSION_MAJOR == 8).
 * P4 长条屏 (p4-bar-dsi) 用 LVGL 9.5 managed component (LVGL_VERSION_MAJOR == 9).
 *
 * LVGL 9 引入了少量破坏性变化, 这里集中收敛:
 *   - lv_point_t → lv_point_precise_t   (lv_line_set_points 等)
 *   - lv_obj_flag_t 严格化           (A | B 不再隐式转换)
 *
 * 卡片代码统一用本垫片提供的 alias / 宏, 避免每个 cpp 各自 #ifdef.
 */

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "lvgl.h"

// C++ 独有部分 (namespace + using). C 文件 include 时跳过.
#ifdef __cplusplus
namespace UI {

#if LVGL_VERSION_MAJOR >= 9
// LVGL 9: lv_line_set_points 接受 lv_point_precise_t
using LinePoint = lv_point_precise_t;
#else
// LVGL 8: 仍是 lv_point_t
using LinePoint = lv_point_t;
#endif

}  // namespace UI
#endif  // __cplusplus

// =============== LVGL 8 → 9 image API 兼容垫片 ===============
// LVGL 9 把 image 相关 API 重命名 (lv_img_* → lv_image_*).
// 卡片代码统一用 LVGL 9 名字, 在 LVGL 8 上自动 alias 到旧名字.
//
// 我们仅在"预缩放 PNG 1:1 居中显示"场景使用 image, 不依赖
// lv_image_set_inner_align/lv_image_set_scale 等 LVGL 9 独有 API,
// 所以垫片只覆盖最小集.
#if LVGL_VERSION_MAJOR < 9

static inline lv_obj_t* lv_image_create(lv_obj_t* parent) {
    return lv_img_create(parent);
}
static inline void lv_image_set_src(lv_obj_t* img, const void* src) {
    lv_img_set_src(img, src);
}

// 类型别名: 让代码可以统一写 lv_image_dsc_t
typedef lv_img_dsc_t lv_image_dsc_t;

// 声明宏: extern const lv_image_dsc_t name;
#ifndef LV_IMAGE_DECLARE
#define LV_IMAGE_DECLARE(name) LV_IMG_DECLARE(name)
#endif

#endif  // LVGL_VERSION_MAJOR < 9

/**
 * lv_obj_clear_flag(obj, A | B) 的 cast — LVGL 9 把 lv_obj_flag_t 收紧后
 * `int` 不能隐式转 enum, 需要 C-style cast. LVGL 8 也接受同一 cast.
 *
 * 用法: lv_obj_clear_flag(obj, LV_FLAGS(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
 */
#define LV_FLAGS(...) ((lv_obj_flag_t)(__VA_ARGS__))

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
