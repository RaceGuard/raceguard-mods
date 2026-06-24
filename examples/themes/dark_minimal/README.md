# Dark Minimal Demo (v0.2.0+ FS 主题示例)

> 状态: ✅ 完整流程演示 (PNG 跟默认主题视觉一样, 仅展示 FS 主题打包/上传/切换流程).
> 你想做真正不同视觉的主题, 自己用 `tools/gauge_bakery/` 烘焙. 见 [`docs/ADD_GAUGE_THEME.md`](../../docs/ADD_GAUGE_THEME.md).

## v0.2.0+ 架构: 主题包走 LittleFS, 不进 firmware

之前 (v0.1.x): 用户主题要写 `gauges.cpp` + 烘焙 .c → 编进 firmware. 一套主题加 ~2MB,
6MB Flash 主分区放不下默认 + 用户主题. **改不了**.

现在 (v0.2.0+): 主题打包成 PNG + manifest.json → 烧 LittleFS partition (4MB, 独立) →
设置菜单 / 代码切换. **firmware 永远 ~5MB, 主题装多少都不挤主分区**.

## 这个 example 包含什么

```
examples/themes/dark_minimal/
└── README.md                          ← 你正在读的, 流程文档

data/themes/dark_minimal/              ← 实际 uploadfs 烧的内容 (mods 仓根)
├── manifest.json                      ← 主题元数据 (name/author/8 gauge 配置)
└── gauge_{coolant,oil_temp,rpm,speed,volts,intake,boost,afr}.png  ← 480×480 RGB PNG
```

烘焙产物已经给你了, **不用动手就能验证流程**:

```bash
cd raceguard-mods
pio run -e round-led-21 -t uploadfs    # 烧 LittleFS (主题包 ~2MB)
```

然后在 main.cpp 加一行测试切换 (设置菜单 GUI 推迟到 v0.2.1):

```cpp
void setup() {
    raceguard::log::init(115200);
    raceguard::hal::platform::initHardware();

    raceguard::backend::startAll();

    // 切到 FS 主题 dark_minimal, 写 NVS, 重启生效
    if (raceguard::ui::theme::select("dark_minimal")) {
        delay(500);
        ESP.restart();
    }
}
```

重启后串口日志:
```
[Theme] 已加载 'dark_minimal' (8 张表)
```

仪表外观跟默认一样 (因为 PNG 是同款), 但 LVGL 走的是 `L:/littlefs/themes/dark_minimal/gauge_*.png`
FS 加载路径. 这就是 FS 主题机制跑通的证据.

## 怎么做你自己的主题包

### 1. 设计 8 张 480×480 PNG

自己用 Figma / PS / 任何工具画 8 张. 命名约定 (跟 manifest 对齐):
```
gauge_coolant.png   gauge_oil_temp.png  gauge_rpm.png    gauge_speed.png
gauge_volts.png     gauge_intake.png    gauge_boost.png  gauge_afr.png
```

设计要点见 [`docs/ADD_GAUGE_THEME.md`](../../docs/ADD_GAUGE_THEME.md) (车内可读性 / 配色对比度 / 角度规范).

### 2. 用 bake_all.py 烘焙 (推荐) 或手写 manifest

#### 用 bake_all (从设计源 PNG 直接出仪表 + manifest)

```bash
cd raceguard-mods
./tools/gauge_bakery/bake_all.py \
    --output-fs cyberpunk_neon \
    --manifest-author "Your Name" \
    --manifest-version "1.0.0"
# 产物: tools/gauge_bakery/output/themes/cyberpunk_neon/
```

#### 或直接放 8 个 PNG + 手写 manifest

```bash
mkdir -p data/themes/cyberpunk_neon
cp my_designed/*.png data/themes/cyberpunk_neon/
# manifest.json 抄本 example 的, 改 name/author/version 即可
```

manifest.json 必填字段:
```json
{
  "name": "Cyberpunk Neon",       // 显示给用户的名字
  "author": "Your Name",
  "version": "1.0.0",
  "format": "raceguard-theme-v1", // 固定值, 不能改 (用于版本兼容检查)
  "screen": "round-led-21",       // 固定值 v0.2.0 只支持圆屏
  "gauges": [
    {
      "name": "COOLANT",          // 必须是 8 个固定槽位之一 (见下)
      "file": "gauge_coolant.png",
      "min": 60.0, "max": 140.0,
      "angle_start_deg": -180.0, "angle_end_deg": 45.0,
      "fmt": "%.0f°C",
      "enabled_default": true
    },
    ...
  ]
}
```

8 个固定槽位 (跟默认主题对齐, v0.2.0 不支持自定义数据源):
`COOLANT / OIL_TEMP / RPM / SPEED / VOLTS / INTAKE / BOOST / AFR`

### 3. 把 example 复制到 data/themes/

```bash
cp -r tools/gauge_bakery/output/themes/cyberpunk_neon data/themes/
```

### 4. 烧 + 切换

```bash
pio run -e round-led-21 -t uploadfs
# main.cpp 内 raceguard::ui::theme::select("cyberpunk_neon") + ESP.restart()
```

## 装多套共存

`data/themes/` 下可以放多个目录,一次 uploadfs 全装上:

```
data/themes/
├── dark_minimal/
├── cyberpunk_neon/
└── jdm_retro/
```

4MB LittleFS 一般够装 ~16 套 (每套 ~2MB). 切换走 `raceguard::ui::theme::select("<id>")`,
设置菜单 GUI 待 v0.2.1 加.

## 切回默认主题

```cpp
raceguard::ui::theme::select("");   // 空 id = 切默认 (.a 内置, 永远兜底)
ESP.restart();
```

## 限制 (v0.2.0)

- ❌ 设置菜单 GUI 入口暂无 (推迟 v0.2.1, 当前必须代码切换)
- ❌ 不支持自定义数据源 (槽位固定 8 个, 想加 STFT_B1 等等 v0.1.3+)
- ❌ 不支持运行时热切 (重启生效, 避免 LVGL 对象重建崩溃)
- ❌ 只支持 round-led-21 (480×480 圆屏), P4 长条屏 v0.2.x 后期

## 参考

- 教程: [`docs/ADD_GAUGE_THEME.md`](../../docs/ADD_GAUGE_THEME.md)
- 烘焙工具: [`tools/gauge_bakery/`](../../tools/gauge_bakery/)
- 公开 API: [`include/raceguard/ui.h`](../../include/raceguard/ui.h) (namespace theme)
