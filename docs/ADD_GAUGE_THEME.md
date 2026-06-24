# 做你自己的仪表主题包 — Step-by-step 指南

> 目标读者: UI 设计师 / 改装爱好者, 想为 ESP32 仪表盘做个性化外观.
> 前提: 已经能 `pio run -e round-led-21 -t upload` 跑通, 默认 8 张通用表能看到.

## 5 步搞定 (~1 天: 半天设计 + 半天烘焙 + 半天集成 + 半天 fine-tune)

### Step 1: 设计 8 张 PNG 底图

主仓默认 8 张表: `COOLANT / RPM / SPEED / VOLTS / INTAKE / AFR / OIL_TEMP / BOOST`.
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
- **角度规范**: 默认 `-180° → +45°` (12 点 = 0°, 顺时针正), 表盘左上为起点, 走 225°. 跟主仓默认表对齐
- **刻度方向一致**: 全部顺时针递增, 红线区在 75%~100%

#### 设计工具

- Figma (推荐, 矢量编辑 + 480×480 frame template)
- Adobe Illustrator / Affinity Designer
- Sketch (Mac)
- 命令行: 喜欢硬核的话, Python + Cairo 直接生成 SVG → PNG

参考主仓默认表的设计源: `tools/gauge_bakery/assets/` (待迁入本仓).

### Step 2: 用烘焙工具生成 LVGL C 数组

```bash
mkdir -p examples/themes/<风格名>/assets
# 把 8 个 PNG 拷进 assets/

./tools/gauge_bakery/bake.py \
    --input  examples/themes/<风格名>/assets/ \
    --output examples/themes/<风格名>/assets/ \
    --prefix gauge_<风格名>_ \
    --size   480
```

产物: 每个 PNG 对应一个 `.c` 文件, 含 `lv_img_dsc_t gauge_<风格名>_<表名>_480` 全局变量.

详细烘焙参数 (色彩格式 / 压缩) 见 [`tools/gauge_bakery/README.md`](../tools/gauge_bakery/README.md).

### Step 3: 复制 dark_minimal 骨架 + 改 namespace

```bash
cp -r examples/themes/dark_minimal examples/themes/<风格名>
```

改 `gauges.cpp`:

```cpp
namespace raceguard_examples::<风格名> {   // 改 namespace

extern "C" {
    extern const lv_img_dsc_t gauge_<风格名>_coolant_480;   // 改 extern 引用
    extern const lv_img_dsc_t gauge_<风格名>_rpm_480;
    // ... 8 张全改
}

const UI::PngGaugeCard::Def kGauges[] = {
    {"COOLANT", &gauge_<风格名>_coolant_480, ...},   // 改 image 指针
    // ...
};
```

### Step 4: 接到 main.cpp 启动流程

```cpp
// src/app/main.cpp
namespace raceguard_examples::<风格名> {
    void registerTheme();
}

void setup() {
    raceguard::log::init(115200);
    raceguard::hal::platform::initHardware();

    raceguard_examples::<风格名>::registerTheme();   // ← 必须在 startAll 之前
    raceguard::backend::startAll();
}
```

`platformio.ini` 把 example 目录加进 build:

```ini
build_src_filter =
    +<*>
    +<../examples/themes/<风格名>/>
```

### Step 5: 烧机验证 + fine-tune

```bash
pio run -e round-led-21 -t upload
```

烧完后, 长按仪表区切换 8 张表, 检查:

- [ ] 数字清晰可读 (车内 50cm 距离, 阳光下/夜间各试一次)
- [ ] 指针指向数字正确 (走一段路, 看 SPEED 表对得上仪表速度)
- [ ] 颜色不偏 (RGB 顺序对了)
- [ ] 没有奇怪的色斑 / 像素 (烘焙参数有问题的话会出现)
- [ ] 切换流畅 (无明显闪烁 / 撕裂)

实拍 2 张图 (`preview-day.png` 日间 + `preview-night.png` 夜间) 放 `examples/themes/<风格名>/`.

### Step 6: 提交 PR

```bash
git checkout -b feat/themes/<风格名>
git add examples/themes/<风格名>/
git commit -m "feat(themes): add <风格名> theme (实设备验证)"
git push origin feat/themes/<风格名>
gh pr create --base main
```

PR description 模板:

```markdown
## 主题信息
- 风格名: cyberpunk_neon
- 设计理念: 赛博朋克霓虹色, 借鉴 GT86 改装常见配色
- 适用场景: 夜间驾驶, 改装车

## 设计要点
- 主配色: #ff00ff (洋红) + #00ffff (青) + #0a0a0a (黑底)
- 字体: Noto Sans Mono (商用字体, 已嵌入)
- 8 张表统一风格 (cohesive design language)

## 实设备测试
- [x] 480×480 圆屏可读性 OK
- [x] 阳光下 (中午 12 点测试) 主数字依然可读
- [x] 夜间车内不刺眼
- [x] 8 张表切换无 lag

## 实拍
- preview-day.png ← 日间车内
- preview-night.png ← 夜间车内
- video-switching.mp4 ← 长按切换演示
```

## 常见坑

### Q: 烘焙完 .c 文件超大 (单张 ~3MB), Flash 装不下

`bake.py` 默认输出 RGB565 (16-bit/px), 480×480 = 460KB/张, 8 张约 3.6MB.
如果觉得大, 选项:
- `--format rgb565_swapped` (跟 LVGL `LV_COLOR_16_SWAP=1` 配合, 不省空间, 只是适配)
- `--format indexed_256` 256 色调色板, 单张降到 ~115KB (但只有 256 色, 不适合渐变)
- 减表数: 不一定要 8 张, 你可以只做 4 张 (COOLANT/RPM/SPEED/VOLTS), Def[] 数组缩短

主仓默认 8 张表已经 ~3.6MB, Flash 16MB partition 留了 6MB 给 firmware, 1 套主题 OK.
不要超过 2 套 (~7MB), 否则不够装.

### Q: 颜色不对, 红的变蓝

RGB 顺序问题. 检查:
- `bake.py --format` 跟 `lv_conf.h` 的 `LV_COLOR_16_SWAP` 对得上
- LVGL 8 上 480×480 ST7701S 用 `LV_COLOR_DEPTH=16 + LV_COLOR_16_SWAP=0`
- 烘焙: `bake.py --format rgb565` (不 swap)

### Q: 字体烘焙到 PNG 还是 LVGL 字体?

**推荐烘焙到 PNG** — 你可以用任何字体 (商用 / 手写 / 字体大全 etc), 设计灵活.

如果非要用 LVGL 矢量字体 (运行时显示当前数值的字体), 见
[LVGL Font Converter](https://lvgl.io/tools/fontconverter), 生成 .c 后塞到
`src/ui/common/fonts/` 并在 main 初始化.

### Q: 我能不能改 8 张表为 12 张 / 4 张 / 1 张?

可以. `kGauges[]` 数组长度自由 (上限 `MAX_GAUGES=16`, 在 PngGaugeCard.h).
但当前所有表共用一个长按轮换序列, 12 张要长按 11 次才能回到第一张, 用户体验差.
建议 ≤8 张.

未来 v0.1.3+ 加追加式 API (`addGauge`), 那时你可以只 register 1-2 张特殊表追加
到默认 8 张后面, 不需要全替换.

### Q: 主题包能和车型 mod 一起用吗?

**完全可以**, 两者正交:
- 车型 mod 改 OBD 数据来源 + DTC 描述
- 主题包改仪表外观

```cpp
void setup() {
    ...
    raceguard_examples::nissan_gtr_r35::registerProfile();    // 车型
    raceguard_examples::cyberpunk_neon::registerTheme();      // 主题
    raceguard::backend::startAll();
}
```

## 参考

- 完整骨架: [`examples/themes/dark_minimal/`](../examples/themes/dark_minimal/)
- 烘焙工具: [`tools/gauge_bakery/`](../tools/gauge_bakery/)
- 公开 API: [`include/raceguard/ui.h`](../include/raceguard/ui.h)
- PngGaugeCard 内部: [`src/ui/common/card/png_gauge_card.h`](../src/ui/common/card/png_gauge_card.h)
