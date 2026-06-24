# Dark Minimal Demo

> ✅ 完整流程演示 — PNG 跟默认主题视觉相同, 仅演示 FS 主题烘焙 / uploadfs / 切换的完整链路.
> 想做不同视觉的主题? 看 [`docs/ADD_GAUGE_THEME.md`](../../../docs/ADD_GAUGE_THEME.md).

## 这个 example 包含

```
data/themes/dark_minimal/    (mods 仓根 data/, PIO uploadfs 烧 LittleFS 用)
├── manifest.json            主题元数据
└── gauge_*.png × 8          480×480 PNG (各 ~250 KB)
```

## 30 秒跑通

```bash
cd raceguard-mods
pio run -e round-led-21 -t uploadfs    # 烧 LittleFS (~10s), firmware 不动
```

main.cpp 加 2 行切到 FS 主题:
```cpp
void setup() {
    raceguard::log::init(115200);
    raceguard::hal::platform::initHardware();
    raceguard::backend::startAll();

    if (strcmp(raceguard::ui::theme::current(), "dark_minimal") != 0) {
        raceguard::ui::theme::select("dark_minimal");
        ESP.restart();   // 写 NVS + 重启生效 (NVS 已记忆, 下次启动直接加载)
    }
}
```

期望串口日志: `[Theme] 已加载 'dark_minimal' (8 张表)`. 仪表外观跟默认相同 (PNG 同款),
但走的是 `L:/littlefs/themes/dark_minimal/gauge_*.png` FS 加载路径 — FS 机制跑通的证据.

## 切回默认主题

```cpp
raceguard::ui::theme::select("");   // 空 id = .a 内置默认 (永远兜底)
ESP.restart();
```

不需要 erase, 默认主题永远在 firmware 内.
