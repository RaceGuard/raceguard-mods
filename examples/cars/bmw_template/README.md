# BMW 起步模板

> 状态: 🟡 模板 (起步代码, 用户自己改自己车的具体型号)

不是完整 mod, 是 **BMW 用户从这里起步** 的脚手架. 主仓 SDK 已经把通用 OBD-II 跑通,
BMW 大部分 SAE 标准 PID 都能直接读, 这个模板只是给你一个"加车型 hint + 启用更多默认表"的起手位.

## 包含什么

| 文件 | 作用 |
|------|------|
| `profile.cpp` | Profile stub — bitmap 默认全开 (不限制 PID), DTC describe 空 stub |
| `gauges_extra.cpp` | 启用 `OIL_TEMP` / `BOOST` 这两张默认 disabled 的仪表 (BMW 一般支持) |

## 用 3 步上手

### 1. 复制到你的 src/app/

```bash
cp -r examples/cars/bmw_template/* src/app/
```

### 2. 改 main.cpp 调注册

```cpp
// src/app/main.cpp
#include <raceguard/backend.h>
#include <raceguard/hal.h>
#include <raceguard/log.h>

namespace raceguard_examples::bmw_template {
    void registerProfile();
    void enableExtraGauges();
}

void setup() {
    raceguard::log::init(115200);
    raceguard::hal::platform::initHardware();

    raceguard_examples::bmw_template::registerProfile();    // 注册 profile (空 hint, 先跑)
    raceguard::backend::startAll();
    raceguard_examples::bmw_template::enableExtraGauges();  // 启用 OIL_TEMP / BOOST
}

void loop() { raceguard::backend::tick(); }
```

### 3. 烧固件实车跑

- 看串口日志, 记下哪些 PID 返回 NO DATA → 回去把它们加到 `kUnsupported[]` 关掉
- 看 DTC 子页, 记下出现的 P1xxx 厂家码 → 用 INPA/ISTA 查描述, 加到 `dtcDescribe()` switch
- 长按仪表区切换, 看 OIL_TEMP / BOOST 是否真的有数 (BMW N20/N55 等涡轮机型)

## 进阶: 写完整 mod

参考 [`../nissan_gtr_r35/`](../nissan_gtr_r35/) — 实车验证后的完整 GT-R adapter, 演示:
- 显式标 7 个不支持的 PID, OBD 调度更高效
- 厂家 DTC 描述 (P1xxx / U1xxx)

## BMW 厂家码资源

- INPA / ISTA-D — 经销商工具, DTC 索引完整
- Bimmerfest / E90Post / BMWblog 论坛
- 部分常见: P1xxx 多为 DME (引擎控制) 错误, U1xxx 是 K-CAN / PT-CAN 通信
