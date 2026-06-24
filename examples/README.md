# RaceGuard Mods Examples

> **设计哲学**: 主仓 `libraceguard-core.a` = "游戏本体" (通用 OBD-II + 8 张通用 PNG 仪表),
> 独立可跑. 本目录 examples 是 **可选附加**, 不装也能用.

## 三种用户路径

### 1. 零代码 (烧官方固件即可)

烧主仓 `pio run -e round-led-21 -t upload` 出的 firmware → 通用 OBD-II 6 张表跑任何车
(COOLANT / RPM / SPEED / VOLTS / INTAKE / AFR). 适合: 想立刻插车上看数据, 不想写代码.

### 2. 启用更多默认表 (1 行代码)

主仓默认 `OIL_TEMP` 和 `BOOST` 两张表是 disabled 状态 (因 GT-R 不支持对应 PID).
你的车支持的话, 在 `main.cpp` 加 1 行:

```cpp
raceguard::ui::setGaugeEnabled(/*idx*/, true);
```

完整例子见 [`cars/bmw_template/gauges_extra.cpp`](cars/bmw_template/gauges_extra.cpp).

### 3. 写完整 car / theme mod

参考下面的 examples, 复制改自己的.

---

## Examples 目录

```
examples/
├── cars/
│   ├── nissan_gtr_r35/     ✅ 实车验证, 完整 GT-R 适配 (bitmap + DTC)
│   ├── bmw_template/       🟡 BMW 用户起步模板 (bitmap 全开 + extra gauges)
│   └── (社区 PR 加更多车型...)
├── themes/
│   ├── dark_minimal/       🟡 暗色极简主题骨架 (代码完整, PNG 自己烘焙)
│   └── (社区 PR 加更多主题...)
└── README.md (本文件)
```

## "覆盖 vs 追加" API 体系

当前 v0.1.2 SDK 提供的是 **覆盖式** API:

| API | 作用 | 模式 |
|------|------|------|
| `raceguard::car::registerProfile()` | 注册车型 hint (bitmap + DTC) | 覆盖 (最后一次注册生效) |
| `raceguard::ui::registerGauges()`   | 替换默认仪表清单 (8 张表) | 覆盖 (要么全用默认, 要么全替) |
| `raceguard::ui::setGaugeEnabled()`  | 运行期开关单张表 (在当前清单里) | 单点修改 |

未来版本 (v0.1.3+) 会加 **追加式** API:
- `raceguard::ui::addGauge(def)` — 追加单张表到默认清单, 不替换
- `raceguard::car::addManufacturerPid(pid)` — 追加单个厂家 PID

当前要追加, 只能通过"先 register 整套 Def[] (含默认 + 自加)"的覆盖路径.

## 怎么贡献

详见 [`cars/README.md`](cars/README.md) 和 [`themes/README.md`](themes/README.md).

简要合并标准:
- ✅ Cars: 实车验证 (≥30 分钟覆盖各工况), README 清晰, 不夹杂私货厂家 PID 直接给到代码 (放 `nullptr` stub + TODO 注释)
- ✅ Themes: 设备实拍效果良好, **不用第三方品牌 logo** (法律风险)
