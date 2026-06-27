# RaceGuard Mods

ESP32 + LVGL 车载 OBD 仪表盘 — 烧上跑任何 OBD-II 车, 想 DIY 就改.

- 🚗 **零配置开跑** — 烧官方固件, 6 张通用表覆盖任意 OBD-II 车
- 🛠️ **加车型 hint** — 30 行 `profile.cpp` 告诉 SDK 你车的 PID 边界 + 厂家 DTC
- 🎨 **换仪表外观** — 烘焙 PNG 主题, 不动 OBD 协议层
- 🌐 **社区共建** — 车型 / 主题 PR 欢迎合入

---

## 📥 下载固件 (不写代码, 直接烧机)

| 版本 | 日期 | 烧机固件 (4.8 MB) | 摘要 |
|------|------|------------------|------|
| **v0.9.0** ⭐ | 2026-06-27 | [📦 raceguard-firmware-v0.9.0-led-21.bin](https://github.com/RaceGuard/raceguard-mods/releases/download/v0.9.0/raceguard-firmware-v0.9.0-led-21.bin) | 修复 SD 卡无法记录 + 仪表切换白屏两个 P0 bug. 双仓编号统一对齐. |

> ⚠️ 旧版 v0.2.0/v0.2.1/v0.2.2 含 SD 死锁 + 仪表白屏 bug, 已下架, 请直接用 v0.9.0.

完整 release notes + SDK .a / sdk-bundle / SHA 校验: 见 [v0.9.0 release 页面](https://github.com/RaceGuard/raceguard-mods/releases/tag/v0.9.0).

**烧机指南** (从零开始, 含硬件选购 + 工具安装 + 触屏激活): [`docs/FLASH_GUIDE.md`](docs/FLASH_GUIDE.md).

---

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
