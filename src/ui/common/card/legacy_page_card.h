#pragma once

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "gauge_card.h"

namespace UI {

/**
 * LegacyPageCard —— 旧页面包装器
 *
 * 把现有 src/ui/led/led_page_*.cpp 风格的"全屏 LVGL 页面"包装成 GaugeCard，
 * 用于 Phase 1 圆屏架构升级过渡期：旧页面零修改、行为完全一致地接入新架构。
 *
 * 使用方式：
 *   LegacyPageCard gball_card(
 *       "GBALL",
 *       []{ GForceMeter::init(); },                  // init_fn
 *       []{ GForceMeter::update(); },                // update_fn
 *       []{ return GForceMeter::g_container; },      // get_container_fn
 *       25                                            // update_ms (40 FPS)
 *   );
 *   gball_card.setOnLongPress([]{ GForceMeter::cycleRange(); });
 *
 * 注意：
 * - LegacyPageCard 假设旧 container 是全屏 480×480。圆屏单卡布局下这是正确的。
 * - 多卡布局（长条屏）需要 reparent + resize，由后续 Phase 中专门处理或重写为原生卡。
 */
class LegacyPageCard : public GaugeCard {
public:
    using LifecycleFn  = void (*)();
    using ContainerFn  = lv_obj_t* (*)();
    using UpdateMsFn   = uint16_t (*)();  // 动态计算 update 间隔（如 GBALL 的 G-Only 模式）

    LegacyPageCard(const char* name,
                   LifecycleFn init_fn,
                   LifecycleFn update_fn,
                   ContainerFn get_container_fn,
                   uint16_t update_ms);

    // GaugeCard 接口
    void onMount(lv_obj_t* parent, const lv_area_t& bounds) override;
    void onUnmount() override;
    void update() override;
    void onShow() override;
    void onHide() override;
    bool onLongPress(int16_t local_x, int16_t local_y) override;

    const char* name() const override { return name_; }
    uint16_t preferredUpdateMs() const override {
        return dynamic_ms_fn_ ? dynamic_ms_fn_() : update_ms_;
    }

    // 旧页面专属可选钩子
    void setOnShow(LifecycleFn fn)         { on_show_ = fn; }
    void setOnHide(LifecycleFn fn)         { on_hide_ = fn; }
    void setOnLongPress(LifecycleFn fn)    { on_long_press_ = fn; }
    void setDynamicUpdateMs(UpdateMsFn fn) { dynamic_ms_fn_ = fn; }

private:
    const char*  name_;
    LifecycleFn  init_fn_;
    LifecycleFn  update_fn_;
    ContainerFn  get_container_fn_;
    uint16_t     update_ms_;

    LifecycleFn  on_show_       = nullptr;
    LifecycleFn  on_hide_       = nullptr;
    LifecycleFn  on_long_press_ = nullptr;
    UpdateMsFn   dynamic_ms_fn_ = nullptr;

    bool initialized_ = false;
};

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
