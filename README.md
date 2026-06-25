# RaceGuard Mods

ESP32 + LVGL 车载 OBD 仪表盘 — 烧上跑任何 OBD-II 车, 想 DIY 就改.

- 🚗 **零配置开跑** — 烧官方固件, 6 张通用表覆盖任意 OBD-II 车
- 🛠️ **加车型 hint** — 30 行 `profile.cpp` 告诉 SDK 你车的 PID 边界 + 厂家 DTC
- 🎨 **换仪表外观** — 烘焙 PNG 主题, 不动 OBD 协议层
- 🌐 **社区共建** — 车型 / 主题 PR 欢迎合入

---

## 📥 下载固件 (不写代码, 直接烧机)

| 版本 | 日期 | 烧机固件 (4.8 MB) | 更新摘要 |
|------|------|------------------|---------|
| **v0.2.0-dev.7** ⭐ | 2026-06-25 | [📦 raceguard-firmware-v0.2.0-dev.7-led-21.bin](https://github.com/RaceGuard/raceguard-mods/releases/download/v0.2.0-dev.7/raceguard-firmware-v0.2.0-dev.7-led-21.bin) | adapter 加 license gate (堵开放层绕过激活) + BUILD_INFO |
| v0.2.0-dev.6 | 2026-06-25 | [📦 v0.2.0-dev.6](https://github.com/RaceGuard/raceguard-mods/releases/download/v0.2.0-dev.6/raceguard-firmware-v0.2.0-dev.6-led-21.bin) | lv_conf.h 进 SDK bundle, 砍双源漂移 |
| v0.2.0-dev.5 | 2026-06-24 | [📦 v0.2.0-dev.5](https://github.com/RaceGuard/raceguard-mods/releases/download/v0.2.0-dev.5/raceguard-firmware-v0.2.0-dev.5-led-21.bin) | 修 PngGaugeCard 底图缺失 (LVGL FS driver 兜底) |
| v0.2.0-dev.4 | 2026-06-24 | [📦 v0.2.0-dev.4](https://github.com/RaceGuard/raceguard-mods/releases/download/v0.2.0-dev.4/raceguard-firmware-v0.2.0-dev.4-led-21.bin) | DEMO 提示改全屏页 + 5s 倒计时 |
| v0.2.0-dev.3 | 2026-06-24 | [📦 v0.2.0-dev.3](https://github.com/RaceGuard/raceguard-mods/releases/download/v0.2.0-dev.3/raceguard-firmware-v0.2.0-dev.3-led-21.bin) | 激活码 SD 卡持久化 |

完整 release notes + SDK .a / sdk-bundle / SHA: 见 [Releases 页面](https://github.com/RaceGuard/raceguard-mods/releases).

**烧机指南** (从零开始, 含硬件选购 + 工具安装 + 触屏激活): [`docs/FLASH_GUIDE.md`](docs/FLASH_GUIDE.md).

---

## 🛠️ 5 分钟跑起来 (要写代码改的话)

> 前提: macOS / Linux + git + python + pio. 不会装看 [`docs/GETTING_STARTED.md`](docs/GETTING_STARTED.md).

```bash
git clone https://github.com/RaceGuard/raceguard-mods.git && cd raceguard-mods
./scripts/fetch_core.sh                            # 拉预编译 .a (~14MB, curl, 不用 gh)
./scripts/new_car.sh honda_civic_fk7               # 生成 mod 框架
vim examples/cars/honda_civic_fk7/profile.cpp      # 30 行业务: PID 边界 + 厂家 DTC
./scripts/enable_mod.sh cars honda_civic_fk7       # 一行启用 (改 active_mods + pio.ini)
./scripts/flash_all.sh                             # 烧 firmware + LittleFS
```

不写代码改的话, 看上面 📥 直接下固件即可.

---

## 三种路径

| 你想 | 怎么做 | 工作量 |
|------|--------|--------|
| 立刻看到我车的数据 | 上面表里下固件烧上 | 0 行 |
| 启用更多通用表 (OIL_TEMP / BOOST) | `setGaugeEnabled(idx, true)` | 1 行 |
| 写完整 car mod | 复制 [`examples/cars/bmw_template/`](examples/cars/bmw_template/) 改 | 30 行 |
| 写主题包 | 复制 [`examples/themes/dark_minimal/`](examples/themes/dark_minimal/) + 烘焙 PNG | 一下午 |

---

## 硬件支持

| 平台 | 显示 | 状态 |
|------|------|------|
| ESP32-S3 + Waveshare Touch LCD 2.1 | 480×480 圆屏 RGB | ✅ 已支持 |
| ESP32-P4 + AXS15260 6.2" DSI | 1280×452 长条屏 | 🚧 规划 |
| M5Paper-S3 EPD | 540×960 e-ink | 🚧 实验 |

---

## 这是什么

闭源核心 `libraceguard-core.a` 封装所有底层 (HAL / OBD 协议栈 / LVGL UI / 激活), 走 GitHub Releases 分发. 你只动 `src/app/` (主入口) + `examples/cars/` (车型适配) + `examples/themes/` (PNG 主题). 完整边界设计: [`CONTRIBUTING.md`](CONTRIBUTING.md).

## 文档

- [`docs/GETTING_STARTED.md`](docs/GETTING_STARTED.md) — 新机器装环境 + 第一次 build
- [`docs/FLASH_GUIDE.md`](docs/FLASH_GUIDE.md) — 不会编程的人怎么烧机
- [`docs/ADD_CAR_PROFILE.md`](docs/ADD_CAR_PROFILE.md) — 加车型适配 5 步
- [`docs/ADD_GAUGE_THEME.md`](docs/ADD_GAUGE_THEME.md) — 烘焙自定义主题
- [`CONTRIBUTING.md`](CONTRIBUTING.md) — PR 流程 + branch/commit 约定

## License

[Apache 2.0](LICENSE) — 怎么用都行. 依赖的 `libraceguard-core.a` 是 proprietary, 不允许反编译 / 二次分发, 详见 [NOTICE](NOTICE).
