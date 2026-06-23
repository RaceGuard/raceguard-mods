#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "legacy_page_card.h"

namespace UI {

LegacyPageCard::LegacyPageCard(const char* name,
                               LifecycleFn init_fn,
                               LifecycleFn update_fn,
                               ContainerFn get_container_fn,
                               uint16_t update_ms)
    : name_(name),
      init_fn_(init_fn),
      update_fn_(update_fn),
      get_container_fn_(get_container_fn),
      update_ms_(update_ms) {}

void LegacyPageCard::onMount(lv_obj_t* parent, const lv_area_t& bounds) {
    (void)parent;
    bounds_ = bounds;

    // 首次挂载：调用旧页面的 init 创建 LVGL 对象
    if (!initialized_) {
        if (init_fn_) init_fn_();
        initialized_ = true;
    }

    // 取回旧 container（init 之后才有效）
    container_ = get_container_fn_ ? get_container_fn_() : nullptr;

    // 显示：清除 HIDDEN flag
    if (container_) {
        lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }

    // 注意：圆屏单卡布局下 bounds 必然是 480×480 全屏，旧 container 默认就是全屏，
    // 无需 reparent / resize。长条屏多卡复用 LegacyPageCard 时需要追加 set_pos/set_size，
    // 由后续 Phase 处理或卡片重写为原生 GaugeCard。
}

void LegacyPageCard::onUnmount() {
    // 隐藏但不销毁，便于快速切回
    if (container_) {
        lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
    }
}

void LegacyPageCard::update() {
    if (update_fn_) update_fn_();
}

void LegacyPageCard::onShow() {
    if (on_show_) on_show_();
}

void LegacyPageCard::onHide() {
    if (on_hide_) on_hide_();
}

bool LegacyPageCard::onLongPress(int16_t local_x, int16_t local_y) {
    (void)local_x; (void)local_y;
    if (on_long_press_) {
        on_long_press_();
        return true;
    }
    return false;
}

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
