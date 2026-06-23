#pragma once

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include <cstdint>
#include <memory>
#include <vector>
#include "lvgl.h"
#include "../card/gauge_card.h"

namespace UI {

/**
 * LayoutEngine —— 卡片编排引擎
 *
 * Phase 1 (圆屏单卡布局)：单 slot 全屏 + 翻页切换激活卡片。
 *   行为等价于现有圆屏 led_ui.cpp 的 Page enum + s_page 管理。
 *
 * Phase 4 (长条屏多卡布局)：N 个 slot 同时显示不同卡片，多 slot API 后续扩展。
 *
 * 卡片所有权：LayoutEngine 拥有卡片实例（unique_ptr）。
 *
 * 触摸事件分发：
 *   1. 布局级手势（边缘滑入设置、左右滑切布局）：由调用方在 onSwipe* 中处理
 *   2. 卡片级事件（onTap / onLongPress）：分发到命中的激活卡片
 */
class LayoutEngine {
public:
    LayoutEngine();
    ~LayoutEngine();

    // ============ 卡片注册 ============

    /** 注册卡片（所有权转移到 engine） */
    void registerCard(std::unique_ptr<GaugeCard> card);

    /** 卡片总数 */
    uint8_t cardCount() const { return (uint8_t)cards_.size(); }

    // ============ 启动 / 激活 ============

    /**
     * 启动布局，挂载激活卡片
     * @param initial_idx  起始激活卡片索引
     * @param parent       父容器（通常 lv_scr_act()）
     */
    void start(uint8_t initial_idx, lv_obj_t* parent);

    /** 当前激活卡片索引（-1 表示未启动） */
    int8_t activeCardIdx() const { return active_idx_; }

    /** 当前激活卡片实例 */
    GaugeCard* activeCard() const;

    /** 当前激活卡片名称（NVS 持久化用） */
    const char* activeCardName() const;

    /** 按名称查找卡片索引（NVS 反查），未找到返回 -1 */
    int8_t findCardByName(const char* name) const;

    // ============ 翻页 ============

    /** 切换到指定卡片（不跳过禁用） */
    void setActiveCard(uint8_t card_idx);

    /** 下一个启用卡片（圆屏左右滑 / 自动跳过禁用卡片） */
    void nextCard();

    /** 上一个启用卡片 */
    void prevCard();

    // ============ 卡片启用控制（如 NAV 暂时关闭） ============

    void setCardEnabled(uint8_t card_idx, bool enabled);
    bool isCardEnabled(uint8_t card_idx) const;

    // ============ 主循环 / 渲染 ============

    /** 按激活卡片的 preferredUpdateMs() 节流调用 card->update() */
    void update();

    /** 强制下次 update 立即执行（跳过节流） */
    void forceUpdate() { last_update_ms_ = 0; }

    /** 隐藏所有卡片（进入设置模式时调用） */
    void hideAll();

    /** 重新显示激活卡片（退出设置模式时调用） */
    void showActive();

    // ============ 触摸事件 ============

    /** 单击事件 → 分发到激活卡片 */
    bool onTap(int16_t x, int16_t y);

    /** 长按事件 → 分发到激活卡片 */
    bool onLongPress(int16_t x, int16_t y);

private:
    std::vector<std::unique_ptr<GaugeCard>> cards_;
    std::vector<bool>                       enabled_;
    int8_t                                  active_idx_     = -1;
    uint32_t                                last_update_ms_ = 0;
    lv_obj_t*                               parent_         = nullptr;

    /** 单卡布局的 bounds（当前 = 全屏，由 ScreenProfile 决定） */
    lv_area_t fullScreenBounds() const;

    /** 把激活卡片切换到 new_idx（带 onShow/onHide 钩子） */
    void switchActiveTo(uint8_t new_idx);
};

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
