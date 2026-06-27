# RaceGuard Core Changelog

记录 SDK / firmware 的关键变化。**只列影响 mods 开发者或最终用户的事项**, 内部重构 / 文档微调不入这里.

格式: 最新版本在最上面.

---

## v0.2.2 — 2026-06-27 (P0 hotfix)

> ⚠️ **强烈建议所有 v0.2.1 用户立即升级**. 本版修复两个独立 P0 regression,
> 均自 v0.2.1 起影响所有用户.

### 修复

**1. SDLogger 初始化死锁 — 行车无 log + SD 菜单全灰显**

v0.2.1 把 `LICENSE_GATE` 加到了所有闭源 method 入口, 但 `backend::startAll`
的调用顺序是:

```
SDLogger::init()  →  LicenseCheck::init()
        ↑
        gate 检查时 isActivated() 必为 false (license 还没加载)
        → SDLogger::init() 直接 return false
        → sdReady 永远 false
        → 所有行车数据无法落盘
        → system 菜单的 SD FORMAT / UPDATE 按钮永久灰显
```

License gate 的多层防御仍由下游 `isReady` / `writeData` / `enqueue` 三处保证,
DEMO 用户依然写不进任何数据 — 移除 `init()` 的 gate 不影响授权保护强度.

**症状识别**: 开机串口日志中**没有** `"正在初始化 SD 卡..."`. 不是硬件问题.

**2. LVGL 图片缓存撑爆 PSRAM — 仪表切换白屏**

`LV_IMG_CACHE_DEF_SIZE = 8` 配置过激进:

- 单张 480×480 PNG decode 后 = 480 × 480 × 4 (ARGB8888) = **900 KB**
  (注: lv_png 解码恒为 ARGB8888, **不**走 RGB565)
- 8 张缓存上限 = **7.2 MB**, 几乎吃光 PSRAM (~7.5 MB)
- 切换到第 N 张 (实测最早从 AFR 触发) → `lv_png` `decoder_open` 报
  `error 83: memory allocation failed` (LODEPNG_OUT_OF_MEMORY) → 白屏

v0.2.2 改为 `LV_IMG_CACHE_DEF_SIZE = 0` (single-slot 模式, 非"关闭缓存"):

- 永远 hold 当前显示这一张 (≤ 900 KB), 指针动 / 屏幕重绘命中, 不重 decode
- 切换瞬间 close 旧的 + open 新的, 单次代价 50-100ms 用户几乎无感
- 占用从 7.2 MB → ≤ 900 KB

**症状识别**: 串口日志中出现:
```
[Warn]  decoder_open: error 83: memory allocation failed (in lv_png.c line #179)
[Warn]  _lv_img_cache_open: Image draw cannot open the image resource
[Warn]  lv_draw_img: Image draw error
```

### 升级方法

mods 用户:

```bash
cd ~/Git/raceguard-mods
./scripts/fetch_core v0.2.2     # 拉新 .a + sdk-bundle
pio run -e round-led-21 -t upload
```

或手动:

```bash
echo "v0.2.2" > CORE_VERSION
# 然后跑 fetch_core 或手动 wget release assets
```

最终用户 (拿到 firmware.bin 的): 用 [网页刷机工具](https://raceguard.cn/flash)
直接刷 `raceguard-firmware-v0.2.2-led-21.bin`.

### 行为变化

- 仪表底图切换 (PngGaugeCard 长按) **从瞬切变为约 50-100ms 解码延迟**,
  这是 cache 收紧的代价. 如果你的场景对延迟敏感, 可在你的 mod fork 里调
  `LV_IMG_CACHE_DEF_SIZE` (但要注意 PSRAM 预算: 每加 1 → 多 900 KB).

### ABI / API

无变化. mods 仓 examples 不需要改代码, 只 bump CORE_VERSION + fetch 新 .a 即可.

---

## v0.2.1 — 2026-06-25

- P0-2 收尾: license gate 下沉到所有闭源 method 入口 (修补 mods 用户可绕过
  adapter 直接 link 底层 method 的漏洞).
- LVGL 精确锁定 8.4.0.

⚠️ **本版引入 v0.2.2 修复的两个 P0 regression**, 不推荐使用.

---

## v0.2.0 — 2026-06-25

- 双仓架构首个正式 release (dev 期收口).
- 主题包走 LittleFS (`ui::theme::` API).
- dark_minimal demo 主题.

---

## 更早版本

dev 期间 (v0.1.x / v0.2.0-dev.N) 不进 CHANGELOG, 仅在 git tag 历史中保留.
