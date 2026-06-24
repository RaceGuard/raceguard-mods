# Dark Minimal 主题 (skeleton)

> 状态: 🟡 骨架 (代码全, PNG 资产 placeholder, 自己烘焙)

演示 **如何换仪表外观, 不动车型 profile**.

## 这套 mod 做了什么

- 用 `raceguard::ui::registerGauges()` 注册 8 张你自己烘焙的 PNG 仪表底图
- 替换 SDK 内置的 8 张默认表 (主仓 `lib/raceguard_core/src/ui/led/assets/gauge_*.png`)
- 数据源 / 阈值 / OBD 协议 *都不变*, 只换视觉

## 包含什么

| 文件 | 状态 |
|------|------|
| `gauges.cpp` | ✅ Def[] 数组 + 注册入口 (引用 `extern const lv_img_dsc_t gauge_dark_*_480` 等) |
| `assets/gauge_dark_*_480.c` | ❌ **缺** — 自己用 `tools/gauge_bakery` 烘焙生成 |
| `assets/gauge_dark_*_480.png` | ❌ **缺** — 自己设计 (Figma / Sketch / PS, 480×480 RGB) |

## 烘焙流程 (5 步)

### 1. 设计 PNG 底图

8 张 480×480 RGB PNG, 表盘背景 + 刻度 + 数字 (不带指针, 指针由 LVGL 矢量绘制).

视觉风格自由, 但建议:
- 黑色背景 (#000-#1a1a1a), 车内夜间不刺眼
- 单色刻度 (#666 或 #aaa), 不喧宾夺主
- 大数字标 0/max, 中间数字小
- 红线区 (RPM/COOLANT/OIL_TEMP) 用 #ff3333 醒目但不过亮

参考 `tools/gauge_bakery/assets/` 看主仓默认风格的素材组织.

### 2. 烘焙 PNG → LVGL C 数组

```bash
cd raceguard-mods
./tools/gauge_bakery/bake.py \
    --input  examples/themes/dark_minimal/assets/ \
    --output examples/themes/dark_minimal/assets/ \
    --prefix gauge_dark_ \
    --size   480
```

产物: 每张 PNG 对应一个 `gauge_dark_*_480.c` (含 `lv_img_dsc_t gauge_dark_*_480` 全局变量).

详细烘焙参数见 [`tools/gauge_bakery/README.md`](../../../tools/gauge_bakery/README.md).

### 3. 加入 build

在 mods 仓 `platformio.ini` 的 `[env:round-led-21]` 段, `build_src_filter` 加一行:

```ini
build_src_filter =
    +<*>
    +<../examples/themes/dark_minimal/>
```

### 4. main.cpp 调注册

```cpp
#include <raceguard/backend.h>

namespace raceguard_examples::dark_minimal {
    void registerTheme();
}

void setup() {
    raceguard::log::init(115200);
    raceguard::hal::platform::initHardware();

    raceguard_examples::dark_minimal::registerTheme();  // ← 必须在 startAll 之前
    raceguard::backend::startAll();
}
```

⚠️ **必须** 在 `backend::startAll()` 之前注册 — `startAll` 内部会调 `raceguard::ui::init()` 构造仪表卡片, 那之后注册晚了 SDK 会跳过.

### 5. 烧固件验证

仪表外观应该变成你的 dark_minimal 风格. 数据 / 长按轮换行为不变.

## 与车型 profile 的关系

主题包 **不动 OBD / DTC / 告警**. 你可以同时用 `examples/cars/<你的车>/` mod 和这个主题:

```cpp
void setup() {
    ...
    raceguard_examples::nissan_gtr_r35::registerProfile();  // 车型 hint
    raceguard_examples::dark_minimal::registerTheme();      // 视觉
    raceguard::backend::startAll();
}
```

## 进阶: 更激进的定制

- 改 8 张表为不同数量 (kGaugeCount 改, 加 / 减 Def)
- 改 `enabled_default` 让某些表初始不出现 (用户自己长按解锁)
- 改 `angle_start_deg` / `angle_end_deg` 让指针跑不同角度范围 (配合不同表盘形状)
- 改 `fmt` 改数字显示格式 (例如 `"%5.1f"` 留 5 字宽)
