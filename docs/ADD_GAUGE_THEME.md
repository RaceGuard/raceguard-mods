# 做你自己的仪表主题包 — Step-by-step 指南

> 目标读者: UI 设计师 / 改装爱好者, 想为 ESP32 仪表盘做个性化外观.
> 前提: 已经能 `pio run -e round-led-21 -t upload` 跑通, 默认 8 张通用表能看到.

## v0.2.0+ 架构: 主题包走 LittleFS, 不进 firmware

| | v0.1.x (旧) | v0.2.0+ (新) |
|---|---|---|
| 主题打包 | 写 `gauges.cpp` + 烘焙 `.c` 数组 | 烘焙 PNG + JSON manifest |
| 编译进哪 | firmware.bin | LittleFS partition (独立) |
| 体积限制 | Flash 6MB 主分区, 1 套主题就超 | LittleFS 4MB, 可装 ~16 套 |
| 切换主题 | 重 build firmware | 设置菜单选 + 重启 (GUI v0.2.1 加, 当前代码切) |
| 多套共存 | ❌ | ✅ |
| 分发 | git PR (代码) | 直接发 .png 包 |

## 5 步搞定 (~1 天: 半天设计 + 半天烘焙 + 1 小时上传/切换/fine-tune)

### Step 1: 设计 8 张 PNG 底图

主仓默认 8 张表: `COOLANT / OIL_TEMP / RPM / SPEED / VOLTS / INTAKE / BOOST / AFR`.
每张 **480×480 RGB PNG**, 表盘背景 + 刻度 + 数字 (不要画指针, 指针由 LVGL 矢量绘制).

```
建议元素 (从外到内):
┌─────────────────────────────┐
│  外圈装饰边 (可选, 1-2px)    │
│  ┌───────────────────────┐  │
│  │  刻度环 (主刻度 + 细分) │  │
│  │  ┌─────────────────┐  │  │
│  │  │  量程数字 0/max  │  │  │
│  │  │   单位标签 °C    │  │  │
│  │  │   表名 COOLANT   │  │  │
│  │  │                  │  │  │
│  │  │  ← 中心留空 (LVGL ← │  │  │
│  │  │     画指针 +     │  │  │
│  │  │     当前数值)     │  │  │
│  │  └─────────────────┘  │  │
│  └───────────────────────┘  │
└─────────────────────────────┘
```

#### 设计要点

- **车内可读**: 主数字 ≥56px, 副数字 ≥24px. 480×480 屏物理直径 5cm, 距离 50cm 看
- **对比度 WCAG AA**: 前景 / 背景对比度 ≥4.5:1 (https://contrast-ratio.com/ 校验)
- **夜间不刺眼**: 主色调浅黑 #0a0a0a~#1a1a1a, 不要纯白 #fff 大色块
- **配色不要超过 3 种主色** + 1 红线区强调色 #ff3333
- **角度规范**: 默认 `-180° → +45°` (12 点 = 0°, 顺时针正), 表盘左上为起点, 走 225°
- **刻度方向一致**: 全部顺时针递增, 红线区在 75%~100%

#### 设计工具

Figma (推荐) / Adobe Illustrator / Affinity Designer / Sketch. 参考主仓默认表的设计源
`tools/gauge_bakery/assets/` (待迁入本仓).

### Step 2: 用烘焙工具出 PNG + manifest.json

```bash
cd raceguard-mods
./tools/gauge_bakery/bake_all.py \
    --output-fs <你的主题名> \
    --manifest-author "Your Name" \
    --manifest-version "1.0.0"
```

产物在 `tools/gauge_bakery/output/themes/<你的主题名>/`:
- `manifest.json` — 主题元数据 (含 8 个槽位配置)
- `gauge_coolant.png` ~ `gauge_afr.png` — 8 张 480×480 PNG (每张 ~250 KB, 全套 ~2 MB)

#### 不用 bake_all? 手写也行

`bake_all` 需要源 PNG 素材 (默认主题用 designer 的 SVG 渲染成 2048×2048 大图再缩放).
你完全可以自己设计 8 张 480×480 PNG, 手写 manifest.json:

```json
{
  "name": "Cyberpunk Neon",      // 显示给用户的名字
  "author": "Your Name",
  "version": "1.0.0",
  "format": "raceguard-theme-v1",  // 固定值, 不能改 (版本兼容标识)
  "screen": "round-led-21",        // 固定值 v0.2.0 只支持圆屏
  "gauges": [
    {
      "name": "COOLANT",           // 必须是 8 个固定槽位之一 (见下)
      "file": "gauge_coolant.png",
      "min": 60.0, "max": 140.0,
      "angle_start_deg": -180.0, "angle_end_deg": 45.0,
      "fmt": "%.0f°C",             // 当前 v0.2.0 该字段被忽略 (用 default), v0.2.1 启用
      "enabled_default": true
    },
    /* ... 共 8 项 */
  ]
}
```

8 个固定槽位 (v0.2.0 不支持自定义数据源, v0.1.3+ 计划扩展):
`COOLANT / OIL_TEMP / RPM / SPEED / VOLTS / INTAKE / BOOST / AFR`

每个槽位都对应一个固定的 CarData 字段 (SDK 内 `theme_data_sources.cpp` 映射),
用户主题只决定"用哪张 PNG + 量程", 不能改"读哪个 PID".

### Step 3: 放进 data/themes/ 目录

```bash
# 推荐: 用 cookie-cutter 一行起步 (cp dark_minimal + 改 manifest.name)
./scripts/new_theme.sh <风格名> "Your Name"
# 然后替换 data/themes/<风格名>/gauge_*.png 成你自己的设计

# 或: 已经用 bake_all 烘焙好了
cp -r tools/gauge_bakery/output/themes/<主题名> data/themes/

# 或: 完全手写
cp -r my_designed_theme data/themes/<主题名>
```

文件结构:
```
raceguard-mods/
└── data/
    └── themes/
        ├── dark_minimal/                          (本仓自带 example)
        │   ├── manifest.json
        │   └── gauge_*.png
        └── <你的主题名>/                          (你新加的)
            ├── manifest.json
            └── gauge_*.png
```

### Step 4: 烧到设备 LittleFS

```bash
pio run -e round-led-21 -t uploadfs
```

注意:
- 这个命令**只烧 LittleFS partition**, 不动 firmware partition. 用户已装的 firmware 不动.
- `data/` 是 PIO 默认 LittleFS 源目录, 整个目录烧进去成为 `/littlefs/` 下的内容
- 重复 uploadfs 会**覆盖** LittleFS 内容 (主题包追加不能用 `-t uploadfs`, 必须每次包含所有主题)

### Step 5: 在代码里切换主题

设置菜单 GUI 推迟到 v0.2.1, v0.2.0 必须代码切:

```cpp
// src/app/main.cpp
#include <raceguard/backend.h>
#include <raceguard/ui.h>

void setup() {
    raceguard::log::init(115200);
    raceguard::hal::platform::initHardware();

    raceguard::backend::startAll();

    // 第一次烧机切换:
    if (strcmp(raceguard::ui::theme::current(), "<你的主题名>") != 0) {
        if (raceguard::ui::theme::select("<你的主题名>")) {
            delay(500);
            ESP.restart();
        }
    }
    // 之后 NVS 已记忆, 启动自动加载
}

void loop() { raceguard::backend::tick(); }
```

烧 firmware 后, 第一次启动会自动切到你的主题并重启. 之后启动直接走主题路径.

期望串口日志:
```
[Theme] 已加载 '<你的主题名>' (8 张表)
```

### Step 6 (可选): 切回原厂主题

```cpp
raceguard::ui::theme::select("");   // 空 id = 切默认 (.a 内置, 永远兜底)
ESP.restart();
```

或者: 写 NVS 工具清空 `rg_theme/current` 字段 (不开发期常用).

## 完整 example

`examples/themes/dark_minimal/` + `data/themes/dark_minimal/` 是完整 example.
PNG 跟默认主题一样 (没设计师出图), 但流程完整跑通可以验证.

复制改名做你自己的:
```bash
cp -r data/themes/dark_minimal data/themes/cyberpunk_neon
# 替换 PNG, 改 manifest.json 的 name/author
```

## 常见问题

### Q: 4MB LittleFS 装不下我所有主题?

单套主题 ~2MB, 理论装 ~2 套就接近. 优化:
1. **256 色 PNG**: 用 `pngquant theme/*.png --quality 85-95 --ext .png --force` 减半 (250KB → 125KB), 装 ~16 套
2. **缩屏精简**: 不画完整 480×480 背景, 只画边缘 + 透明中心, 减重大量
3. **改分区表**: `partitions_16mb_lfs.csv` 调 littlefs 大小. 但缩小别的分区有代价 (app/ota_0 是 firmware 用的)

### Q: 切换后屏幕黑了

- 主题 PNG 文件损坏 / manifest 字段错 → 串口看 `[Theme]` 日志
- 自救: `raceguard::ui::theme::select("")` + 重启 → 回默认 (代码或 serial cmd)
- 实在不行: 物理擦除 NVS `esptool.py erase_region 0x9000 0x5000` → 默认配置重启

### Q: PNG 解码慢?

LVGL lodepng 解 480×480 RGB PNG ~50-150ms. LVGL `LV_IMG_CACHE_DEF_SIZE = 8` 缓存 8 张解码后位图,
首次切表慢, 后续轮换瞬时 (cache hit).

### Q: 我能不能动 angle / fmt / enabled_default 等字段?

当前 v0.2.0:
- `min` / `max` / `angle_start_deg` / `angle_end_deg` — ✅ 真生效
- `enabled_default` — ✅ 真生效
- `fmt` — ⚠️ 被忽略, 用 default (跟默认主题一致). v0.2.1 启用 (要解决字符串生命周期)
- `name` — 必须是 8 个固定槽位之一, 别的会被忽略

### Q: 主题包能跟车型 mod 一起用吗?

**正交, 完全可以**:
- 车型 mod (`examples/cars/*/profile.cpp`) 改 OBD 数据来源 + DTC 描述
- 主题包 (`data/themes/*/`) 改仪表外观

```cpp
void setup() {
    raceguard_examples::nissan_gtr_r35::registerProfile();   // 车型
    raceguard::backend::startAll();
    raceguard::ui::theme::select("cyberpunk_neon");           // 主题
    ESP.restart();
}
```

### Q: 老 (v0.1.x) `gauges.cpp` + `registerGauges()` 方案还能用吗?

**还能, 但不推荐**. `registerGauges()` API 保留兼容, 用户自定义 `Def[]` (含 `&lv_img_dsc_t image` 字段) 仍然 link 进 firmware. 但 Flash 容量限制还在, 一套主题加完就接近上限.

新主题强烈建议走 FS 路径 (体积无限制, 多套共存).

## 参考

- 完整 example: [`examples/themes/dark_minimal/`](../examples/themes/dark_minimal/)
- 烘焙工具: [`tools/gauge_bakery/`](../tools/gauge_bakery/)
- 公开 API: [`include/raceguard/ui.h`](../include/raceguard/ui.h) (`namespace theme`)
- 主题数据源映射 (槽位 → CarData 字段): SDK 内部 `theme_data_sources.cpp`

---

## 目录约定 / 命名规则

```
data/themes/<风格名>/                                (实际烧 LittleFS 的内容)
├── manifest.json
└── gauge_{coolant,oil_temp,rpm,speed,volts,intake,boost,afr}.png
```

风格名: 全小写, 下划线分隔, 描述视觉感.

```
✅ dark_minimal        (暗色极简)
✅ cyberpunk_neon      (赛博朋克霓虹)
✅ jdm_retro           (JDM 复古机械)
✅ digital_modern      (现代数字)
❌ MyTheme             (要全小写)
❌ classic             (太泛, 多人撞名)
```

## 合并标准 (PR Review 时检查)

- ✅ **设备实拍** preview-day.png + preview-night.png (车内白天/夜间各拍一张, 不是 mockup)
- ✅ 480×480 圆屏可读性: 主数字 ≥56px, 副数字 ≥24px
- ✅ 配色对比度 WCAG AA (≥4.5:1, 用 https://contrast-ratio.com/ 校验)
- ✅ 8 张表统一风格 (cohesive design language, 不是凑数)
- ❌ **不用第三方品牌商标** — 不能用真实 Ferrari / Porsche / BMW / GT-R logo (法律风险, 直接 reject)
- ❌ 不照搬其他商业产品 UI (法律风险)
- ❌ 不暗藏二维码 / 链接等隐性宣传
