#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "screen_profile.h"

namespace UI {

bool ScreenProfile::isInsideVisibleArea(int16_t x, int16_t y) const {
    // 物理范围
    if (x < 0 || y < 0) return false;
    if (x >= (int16_t)width || y >= (int16_t)height) return false;

    // Safe area
    if (x < safe_x || y < safe_y) return false;
    if (x >= safe_x + (int16_t)safe_w) return false;
    if (y >= safe_y + (int16_t)safe_h) return false;

    // 形状判定
    switch (shape) {
        case ShapeKind::RECT:
            return true;

        case ShapeKind::CIRCLE: {
            int32_t cx = width / 2;
            int32_t cy = height / 2;
            int32_t r  = (width < height) ? width / 2 : height / 2;
            int32_t dx = x - cx;
            int32_t dy = y - cy;
            return (dx * dx + dy * dy) <= r * r;
        }

        case ShapeKind::ROUNDED_RECT: {
            int32_t r = corner_radius;
            if (r <= 0) return true;

            // 四角圆弧判定：先定位 (x,y) 落在哪个圆角参考圆心
            int32_t cx, cy;
            bool need_check = false;

            if (x < r) {
                cx = r;
                need_check = true;
            } else if (x >= (int16_t)width - r) {
                cx = (int32_t)width - r - 1;
                need_check = true;
            } else {
                cx = x;
            }

            if (y < r) {
                cy = r;
            } else if (y >= (int16_t)height - r) {
                cy = (int32_t)height - r - 1;
            } else {
                if (!need_check) return true;  // 中间矩形带，y 不在圆角区
                cy = y;
            }

            if (!need_check && (y >= r && y < (int16_t)height - r)) return true;

            int32_t dx = x - cx;
            int32_t dy = y - cy;
            return (dx * dx + dy * dy) <= r * r;
        }

        case ShapeKind::OCTAGON:
        case ShapeKind::CUSTOM_MASK:
            // TODO: 实现八边形 / mask 判定（暂按 RECT 处理）
            return true;
    }
    return true;
}

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
