#pragma once

#if defined(DISPLAY_TYPE_LED) || defined(DISPLAY_TYPE_P4_BAR)

#include "gauge_card.h"
#include "../lv_compat.h"
#include "types/car_data.h"
#include <cstdint>

namespace UI {

/**
 * PngGaugeCard —— PNG 烘焙底图 + 矢量动态层的通用仪表卡片
 *
 * 视觉策略：
 *   - PNG 底图（tools/gauge_bakery 生成）承担：背景、刻度、数字、单位、
 *     标题、告警色带 —— 一切静态内容
 *   - LVGL 运行时只画：3 层 glow 指针 + 中央实时数值 label
 *
 * 多表轮换：
 *   - 构造时传入 Def 数组（每个 Def = 一张表的配置）
 *   - 长按切换到下一个 enabled_default=true 的表
 *   - enabled_default=false 的表（如 GT-R 不支持的 OIL_TEMP / BOOST）跳过
 */
class PngGaugeCard : public GaugeCard {
public:
    /** 单张仪表的描述 */
    struct Def {
        const char*           name;             // "COOLANT"
        const lv_image_dsc_t* image;            // 底图（gauges/gauge_*.c 提供，LVGL 8 下 alias 到 lv_img_dsc_t）
        float                 value_min;
        float                 value_max;
        // 钟表坐标：12 点 = 0°，顺时针正。烘焙脚本默认 -180 → +45
        float                 angle_start_deg;
        float                 angle_end_deg;
        float (*getter)(const CarData&);        // 从 latestData 取值
        bool  (*has_value)(const CarData&);     // 检查字段是否有效
        const char*           fmt;              // 数值格式串，如 "%.0f"
        bool                  enabled_default;  // 是否参与长按轮换
    };

    PngGaugeCard(const Def* defs, uint8_t count, uint8_t default_idx);
    ~PngGaugeCard() override;

    void onMount(lv_obj_t* parent, const lv_area_t& bounds) override;
    void onUnmount() override;
    void update() override;
    bool onLongPress(int16_t local_x, int16_t local_y) override;

    const char* name() const override;
    // 数据源 OBD 5-10Hz，UI 20FPS 跑 EMA 平滑插值让指针动得丝滑
    uint16_t preferredUpdateMs() const override { return 50; }

private:
    const Def* defs_  = nullptr;
    uint8_t    count_ = 0;
    uint8_t    cur_idx_ = 0;

    // ── LVGL 对象 ──
    lv_obj_t* bg_img_      = nullptr;
    lv_obj_t* needle_body_ = nullptr;
    lv_obj_t* needle_core_ = nullptr;
    lv_obj_t* val_label_   = nullptr;

    // ── 几何（按 bounds 在 createObjects 中计算） ──
    int16_t cx_ = 0, cy_ = 0;
    int16_t needle_root_r_  = 0;
    int16_t needle_tip_r_   = 0;
    int16_t val_label_right_x_  = 0;  // 数值 label 框的右边 x
    int16_t val_label_bottom_y_ = 0;  // 数值 label 框的底边 y

    // ── 状态 ──
    float     needle_smooth_ = 0.0f;  // EMA 平滑的归一化值 0-1
    char      last_drawn_text_[16] = {};
    bool      last_drawn_invalid_ = false;  // 上次画的是 "---"
    LinePoint needle_pts_[2] = {{0,0},{0,0}};
    int16_t   needle_obj_x_ = -32768;  // 指针 line 对象左上角（让 invalidate 区域跟随指针）
    int16_t   needle_obj_y_ = -32768;

    // ── 私有方法 ──
    const Def* currentDef() const;
    uint8_t    nextEnabledIdx() const;
    void       createObjects(lv_obj_t* parent);
    void       destroyObjects();
    void       updateNeedle(float pct);
    void       switchTo(uint8_t idx);
};

}  // namespace UI

#endif  // DISPLAY_TYPE_LED || DISPLAY_TYPE_P4_BAR
