# `include/raceguard/` — RaceGuard SDK 公开 API

预编译核心库 (`libraceguard-core-<env>.a`) 通过这些 header 暴露功能.
用户代码 `#include <raceguard/...>` 即可使用.

## 文件

| Header | 命名空间 | 提供 |
|--------|----------|------|
| `version.h` | `raceguard::` | 版本号常量 |
| `log.h` | `raceguard::log` | 日志宏 `RG_LOG_INFO/WARN/ERROR/DEBUG` |
| `data.h` | `raceguard::data` | `latest()` / `sessionPeaks()` getter (访问最新 OBD 数据) |
| `hal.h` | `raceguard::hal::{display,touch,imu,nvs,platform}` | 硬件抽象 |
| `obd.h` | `raceguard::obd` | OBD 后端管理 (BLE / CAN) |
| `car.h` | `raceguard::car` | 车型 profile 注册 |
| `alert.h` | `raceguard::alert` | 告警引擎查询 |
| `storage.h` | `raceguard::storage` | SD 日志异步写入 |

## API 稳定性承诺

| 版本范围 | 承诺 |
|----------|------|
| v0.x | **不稳定**. 任何 minor bump 都可能改 header. |
| v1.0+ | **稳定**. 添加新函数 OK, 删除 / 改签名需 major bump. |

## 怎么用

```cpp
#include <raceguard/data.h>
#include <raceguard/log.h>

void loop() {
    auto& d = raceguard::data::latest();
    if (raceguard::data::CarData::hasValue(d.rpm)) {
        RG_LOG_INFO("RPM = %d", d.rpm);
    }
}
```

具体调用示例见 `src/app/main.cpp`.
