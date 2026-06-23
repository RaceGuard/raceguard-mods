# RaceGuard Mods

一个 ESP32 + LVGL 车载 OBD 仪表盘 DIY 项目.

- 🚗 **适配你的车** — 写 OBD PID 解析 + DTC 描述, 让仪表读懂你的车
- 🎨 **定制仪表风格** — 用烘焙工具做自己的仪表盘 PNG, 或写完全自定义的卡片
- 🌐 **社区共建** — 优质适配 / 仪表 PR 欢迎合入

> ⚠️ **状态: WIP** — 首批 UI 框架 + 烘焙工具链已迁入, build 配置补齐中 (v0.1 发布). Star 追进展.

---

## 这是什么

针对 ESP32 (S3 / P4) 车载仪表盘的 DIY SDK. 我们封装好了所有底层硬件驱动 + OBD 协议栈 +
告警引擎, 你只需关心**上层**:

| 模块 | 你能改 / 写的 |
|------|------|
| 🛠️ **UI 框架** | `GaugeCard / LayoutEngine / PngGaugeCard` 基类, 写自己的仪表卡片 |
| 🎨 **烘焙工具链** | 把设计图烘焙成 LVGL 仪表底图 |
| 📋 **车型 profile** | 注册你的车的 OBD PID 列表 / DTC 描述表 |
| 📚 **示例参考** | GT-R 车型适配 + 多套仪表风格作为模板 |

底层 (HAL 实测调优 / OBD 协议栈 / SD 日志 / 告警引擎) 通过 GitHub Releases 分发
**预编译核心库** `libraceguard-core-<env>.a`, 由 `scripts/fetch_core.sh` 自动拉取后
和你的代码一起 link 出 firmware. 不用关心硬件细节, 不用调 PSRAM / DMA 时序.

## 谁应该用

- **ESP32 爱好者** — 想玩车载仪表盘 DIY
- **UI 设计师 / 改装爱好者** — 想为自己的车做个性化仪表
- **车圈开发者** — 适配自己的车 (BMW / 本田 / 大众 ...) 加入社区车型库

## 快速上手 (v0.1 发布后)

```bash
git clone https://github.com/RaceGuard/raceguard-mods.git
cd raceguard-mods
./scripts/fetch_core.sh                # 拉预编译核心库
pio run -e round-led-21                # 编 firmware
pio run -e round-led-21 -t upload      # 烧到 ESP32
```

详细教程见 `docs/` (待 v0.1 发布).

## 硬件支持

| 平台 | 显示 | 状态 |
|------|------|------|
| ESP32-S3 + Waveshare Touch LCD 2.1 | 480×480 圆屏 RGB | ✅ 已支持 |
| ESP32-P4 + AXS15260 6.2" DSI | 1280×452 长条屏 | 🚧 开发中 |
| M5Paper-S3 EPD | 540×960 e-ink | 🚧 实验 |

完整 BOM 见 [`docs/hardware.md`](docs/hardware.md) (待发布).

## 已就绪内容

```
src/
├── ui/common/                      UI 框架 (LVGL 8/9 双兼容)
│   ├── card/
│   │   ├── gauge_card.h            卡片基类抽象
│   │   ├── png_gauge_card.{h,cpp}  PNG 底图 + LVGL 动态层仪表
│   │   ├── digital_card.{h,cpp}    纯数字卡片
│   │   ├── fuel_trim_card.{h,cpp}  STFT/LTFT 双 bank 修正
│   │   ├── timing_card.{h,cpp}     点火提前角
│   │   ├── gauge_meter_card.{h,cpp} lv_scale 原生圆盘仪表
│   │   └── legacy_page_card.{h,cpp} 整页直接绘图适配壳
│   ├── layout/layout_engine.{h,cpp} 卡片布局引擎
│   ├── screen_profile.{h,cpp}      屏幕尺寸/方向抽象
│   └── lv_compat.h                 LVGL 8/9 API 兼容垫片
├── types/car_data.h                CarData 数据契约 + SessionPeaks
└── comm/mock_obd_feed.{h,cpp}      Mock OBD 数据源 (无车 / FPS 压测用)

tools/
├── gauge_bakery/                   仪表 PNG 烘焙流水线
└── png_to_lvgl9.py                 PNG → LVGL 9 RGB888 C 数组

include/raceguard/                  SDK 公开 API (核心库通过这些 header 提供功能)
```

## 路线图

- [x] **v0.0.x** — UI 框架代码 + 烘焙工具链代码迁入 (无 build 配置, 本仓当前状态)
- [ ] **v0.1.0** — 完整 build 配置 + 核心库首发 + 示例车型 (GT-R) + 示例仪表风格
- [ ] **v0.2.0** — 自定义卡片 API + 完整 SDK 文档
- [ ] **v0.3.0** — 多平台 (P4 长条屏) 支持
- [ ] **v1.0.0** — 稳定 API 承诺

## 贡献

车型适配 / 仪表风格 PR **欢迎**!

- **车型适配** 提交到 [`examples/cars/<品牌_车型>/`](examples/cars/)
- **仪表风格** 提交到 [`examples/themes/<风格名>/`](examples/themes/)
- 经过实车 / 实设备验证 + code review 后合并
- 详细贡献指南: `CONTRIBUTING.md` (待发布)

Bug / 功能建议: [GitHub Issues](https://github.com/RaceGuard/raceguard-mods/issues).

## License

[Apache License 2.0](LICENSE) — 怎么用都行.
