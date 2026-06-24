# RaceGuard Mods

ESP32 + LVGL 车载 OBD 仪表盘 — **任何 OBD-II 车开箱通用**, mods 是可选扩展.

- 🚗 **零配置开跑** — 烧官方固件, 6 张通用表跑任何 OBD-II 车
- 🛠️ **加车型 hint** — 写 30 行 `profile.cpp`, 告诉 SDK 你的车的 PID 边界 + 厂家 DTC
- 🎨 **换仪表外观** — 烘焙自己的 PNG 主题, 不动 OBD 协议层
- 🌐 **社区共建** — 优质车型 / 主题 PR 欢迎合入

> 状态: **v0.2.0-dev** — 主题包走 LittleFS (不挤 Flash); examples + 治理文档完整; public release ready.

---

## 三种用户路径

| 你想要 | 怎么做 | 工作量 |
|--------|--------|--------|
| **立刻看到我的车的数据** | 烧官方固件 (链到 GitHub Releases) | 0 行代码 |
| **启用更多表 (OIL_TEMP / BOOST)** | `setGaugeEnabled(idx, true)` | 1 行代码 |
| **写完整 car mod** | 复制 [`examples/cars/bmw_template/`](examples/cars/bmw_template/) 改 | 30 行 |
| **写主题包** | 复制 [`examples/themes/dark_minimal/`](examples/themes/dark_minimal/) + 烘焙 PNG | 一下午 |

## 这是什么

针对 ESP32 (S3 / P4) 车载仪表盘的 DIY SDK. 闭源核心库 `libraceguard-core.a`
封装了**所有底层** (HAL / OBD 协议栈 / 告警引擎 / LVGL UI / 激活), 通过 GitHub Releases
分发. 用 `scripts/fetch_core.sh` 拉到本地, 和你的代码 link 出 firmware.

| 闭源 (核心库) | 开源 (本仓) |
|------|------|
| HAL 实测调优 (DMA / PSRAM / 二阶段启动) | UI 框架 (`PngGaugeCard / LayoutEngine` 基类) |
| OBD 协议栈 (ELM327 BLE / CAN / PID 调度) | 烘焙工具链 (`tools/gauge_bakery/`) |
| SAE J1979 标准 PID 解析 | 车型适配 examples (`examples/cars/`) |
| SAE J2012 P0xxx DTC 描述表 (86 条) | 主题包 examples (`examples/themes/`) |
| AlertMonitor 告警引擎 + 激活 | `main.cpp` 主入口 (~20 行) |
| 通用 8 张 PNG 仪表 | — |

完整边界设计见主仓 `docs/dev/raceguard-sdk-boundary.md`.

## 谁应该用

- **ESP32 爱好者** — 想玩车载仪表盘 DIY
- **UI 设计师 / 改装爱好者** — 给自己的车做个性化仪表
- **车圈开发者** — 适配自己的车 (BMW / 本田 / 大众 ...) 加入社区库

## 快速上手

**已有 PIO + ESP32 开发环境的话**:

```bash
git clone https://github.com/RaceGuard/raceguard-mods.git
cd raceguard-mods
./scripts/fetch_core.sh v0.2.0-dev     # 拉预编译 .a (~13MB) + headers + sha256
pio run -e round-led-21                # 编 firmware
pio run -e round-led-21 -t upload      # 烧到 ESP32
```

**第一次接触 ESP32 / PIO?** 看 [`docs/GETTING_STARTED.md`](docs/GETTING_STARTED.md) — 从装 brew/PIO 开始,涵盖 Mac/Linux/Windows,常见坑都列了.

烧完后:
- 通用 ESP32 → DEMO 模式 (mock 数据驱动 UI, 演示用)
- 商业盒子 (NVS 预写激活码) → ACTIVATED 模式 (全功能, 接 OBD 真实数据)
- 设置菜单点 **ACTIVATE** → 手机连 WiFi 输 16 位激活码 → 重启进 ACTIVATED

## 硬件支持

| 平台 | 显示 | 状态 |
|------|------|------|
| ESP32-S3 + Waveshare Touch LCD 2.1 | 480×480 圆屏 RGB | ✅ 已支持 |
| ESP32-P4 + AXS15260 6.2" DSI | 1280×452 长条屏 | 🚧 v0.1.3+ |
| M5Paper-S3 EPD | 540×960 e-ink | 🚧 实验 |

## 仓库结构

```
src/
├── app/
│   └── main.cpp                    主入口 (~20 行, 调 backend::startAll/tick)
├── ui/common/                      UI 框架 (LVGL 8/9 双兼容, 与主仓 sync)
│   ├── card/                       GaugeCard 基类 + 6 种卡片 (PNG / Digital / FuelTrim / Timing / Meter / LegacyPage)
│   ├── layout/layout_engine.{h,cpp}
│   ├── screen_profile.{h,cpp}
│   └── lv_compat.h                 LVGL 8/9 API 兼容垫片
└── types/car_data.h                CarData 数据契约 + SessionPeaks

include/raceguard/                  SDK 公开 API (链到核心库)
├── log.h     hal.h      data.h     版本号 / 日志 / 硬件 / 数据访问
├── obd.h     car.h      storage.h  OBD / 车型 profile / 存储
├── alert.h   ui.h       backend.h  告警 / UI / 主入口
├── license.h demo.h                激活 / 演示 (DEMO 模式 mock 数据)
└── version.h

examples/
├── cars/
│   ├── README.md                   贡献区目录约定
│   ├── nissan_gtr_r35/             ✅ 实车验证, 完整 GT-R 适配
│   ├── bmw_template/               🟡 BMW 起步模板 + extra gauges
│   └── README.md                   examples 总览 + "覆盖 vs 追加" API 体系
└── themes/
    ├── README.md
    └── dark_minimal/               🟡 暗色极简骨架 (代码全, PNG 自己烘焙)

tools/
├── gauge_bakery/                   仪表 PNG → LVGL C 数组
└── png_to_lvgl9.py                 PNG → LVGL 9 RGB888

lib/raceguard_core/                 .gitignore 忽略, fetch_core.sh 拉 .a 到此
scripts/
├── fetch_core.sh                   从 Releases 拉预编译 .a
└── link_core.py                    pio extra_script, 按 env 名匹配 .a
```

## 路线图

- [x] **v0.0.x** — UI 框架代码 + 烘焙工具链代码迁入
- [x] **v0.1.0-dev** — build 配置 + 核心库首发 (本地 .a)
- [x] **v0.1.1-dev** — 双仓 main.cpp 1:1 + LED UI 入 .a + DEMO/ACTIVATED 双模
- [x] **v0.1.2-dev** — `registerProfile` 真接通 + examples (GT-R / BMW / Dark) + 文档
- [x] **v0.2.0-dev** — 主题包走 LittleFS (不挤 Flash), `theme::list/select/current` API, dark_minimal demo
- [ ] **v0.2.1** — 设置菜单 Theme 子页 GUI (当前 v0.2.0 须代码切换)
- [ ] **v0.1.3 / v0.3.0** — 追加式 API + 自定义数据源 (任意 CarData 字段做主题)
- [ ] **v0.4.0** — P4 长条屏完整支持
- [ ] **v1.0.0** — 稳定 API 承诺

## 贡献

- **车型适配** → [`examples/cars/<品牌_车型>/`](examples/cars/), 参考 `nissan_gtr_r35` 模板
- **仪表风格** → [`examples/themes/<风格名>/`](examples/themes/), 用 `tools/gauge_bakery/` 烘焙
- 实车 / 实设备验证 + code review 后合并

详细贡献指南: `CONTRIBUTING.md` (v0.2 发布).

Bug / 功能建议: [GitHub Issues](https://github.com/RaceGuard/raceguard-mods/issues).

## License

[Apache 2.0](LICENSE) — 怎么用都行.

依赖的 `libraceguard-core-<env>.a` 是 **proprietary**, 不允许反编译 / 二次分发,
详见 [NOTICE](NOTICE).
