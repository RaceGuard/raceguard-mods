#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "layout_engine.h"
#include "../screen_profile.h"
#include <Arduino.h>
#include <cstring>

namespace UI {

LayoutEngine::LayoutEngine() = default;
LayoutEngine::~LayoutEngine() = default;

void LayoutEngine::registerCard(std::unique_ptr<GaugeCard> card) {
    if (!card) return;
    cards_.push_back(std::move(card));
    enabled_.push_back(true);
}

void LayoutEngine::start(uint8_t initial_idx, lv_obj_t* parent) {
    parent_ = parent ? parent : lv_scr_act();
    if (cards_.empty()) return;

    // 跳过禁用，找到第一个启用卡片
    uint8_t idx = initial_idx;
    if (idx >= cards_.size()) idx = 0;
    if (!enabled_[idx]) {
        for (uint8_t i = 0; i < cards_.size(); ++i) {
            uint8_t candidate = (idx + i) % cards_.size();
            if (enabled_[candidate]) { idx = candidate; break; }
        }
    }

    switchActiveTo(idx);
}

GaugeCard* LayoutEngine::activeCard() const {
    if (active_idx_ < 0 || active_idx_ >= (int8_t)cards_.size()) return nullptr;
    return cards_[active_idx_].get();
}

const char* LayoutEngine::activeCardName() const {
    GaugeCard* c = activeCard();
    return c ? c->name() : "";
}

int8_t LayoutEngine::findCardByName(const char* name) const {
    if (!name) return -1;
    for (size_t i = 0; i < cards_.size(); ++i) {
        if (strcmp(cards_[i]->name(), name) == 0) return (int8_t)i;
    }
    return -1;
}

void LayoutEngine::setActiveCard(uint8_t card_idx) {
    if (card_idx >= cards_.size()) return;
    if (card_idx == active_idx_) return;
    switchActiveTo(card_idx);
}

void LayoutEngine::nextCard() {
    if (cards_.empty() || active_idx_ < 0) return;
    uint8_t next = active_idx_;
    for (uint8_t i = 0; i < cards_.size(); ++i) {
        next = (next + 1) % cards_.size();
        if (enabled_[next]) break;
    }
    if (next != active_idx_) switchActiveTo(next);
}

void LayoutEngine::prevCard() {
    if (cards_.empty() || active_idx_ < 0) return;
    uint8_t prev = active_idx_;
    for (uint8_t i = 0; i < cards_.size(); ++i) {
        prev = (prev + cards_.size() - 1) % cards_.size();
        if (enabled_[prev]) break;
    }
    if (prev != active_idx_) switchActiveTo(prev);
}

void LayoutEngine::setCardEnabled(uint8_t card_idx, bool enabled) {
    if (card_idx >= enabled_.size()) return;
    enabled_[card_idx] = enabled;

    // 如果禁用的是当前激活卡片，自动跳到下一个启用卡片
    if (!enabled && active_idx_ == (int8_t)card_idx) {
        nextCard();
    }
}

bool LayoutEngine::isCardEnabled(uint8_t card_idx) const {
    if (card_idx >= enabled_.size()) return false;
    return enabled_[card_idx];
}

void LayoutEngine::update() {
    GaugeCard* c = activeCard();
    if (!c) return;

    uint32_t now = millis();
    uint32_t interval = c->preferredUpdateMs();
    if (interval > 0 && (now - last_update_ms_) < interval) return;
    last_update_ms_ = now;

    c->update();
}

void LayoutEngine::hideAll() {
    for (auto& card : cards_) {
        card->onHide();
        card->onUnmount();
    }
}

void LayoutEngine::showActive() {
    GaugeCard* c = activeCard();
    if (!c) return;
    c->onMount(parent_, fullScreenBounds());
    c->onShow();
    forceUpdate();
}

bool LayoutEngine::onTap(int16_t x, int16_t y) {
    GaugeCard* c = activeCard();
    if (!c) return false;
    // 单卡满屏布局：local 坐标 = 屏幕坐标
    return c->onTap(x - c->bounds().x1, y - c->bounds().y1);
}

bool LayoutEngine::onLongPress(int16_t x, int16_t y) {
    GaugeCard* c = activeCard();
    if (!c) return false;
    return c->onLongPress(x - c->bounds().x1, y - c->bounds().y1);
}

lv_area_t LayoutEngine::fullScreenBounds() const {
    const ScreenProfile& p = currentProfile();
    lv_area_t a;
    a.x1 = 0;
    a.y1 = 0;
    a.x2 = (lv_coord_t)p.width - 1;
    a.y2 = (lv_coord_t)p.height - 1;
    return a;
}

void LayoutEngine::switchActiveTo(uint8_t new_idx) {
    if (new_idx >= cards_.size()) return;

    // 当前卡片下台
    if (active_idx_ >= 0) {
        GaugeCard* prev = cards_[active_idx_].get();
        if (prev) {
            prev->onHide();
            prev->onUnmount();
        }
    }

    // 新卡片上台
    active_idx_ = (int8_t)new_idx;
    GaugeCard* next = cards_[new_idx].get();
    if (next) {
        next->onMount(parent_, fullScreenBounds());
        next->onShow();
    }

    forceUpdate();
}

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
