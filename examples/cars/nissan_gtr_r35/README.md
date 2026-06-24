# Nissan GT-R R35 完整适配

> 状态: ✅ 实车验证 (2026, R35 MY12+)

完整的车型 mod 示例, 演示 `raceguard::car::Profile` 两个核心字段的写法.

## 这个 mod 做了什么

1. **PID 调度优化** — 告诉 SDK GT-R 不支持 `0x0A / 0x0B / 0x10 / 0x2F / 0x33 / 0x46 / 0x5C` 这 7 个 PID, OBD 调度跳过这些, 减少超时浪费.
2. **GT-R 厂家 DTC 描述** — 处理 P1xxx / U1xxx 厂家故障码 (`P1217 VR38 引擎过热保护` / `U1218 CAN 总线 TCU 通信丢失` 等), 没命中的 fallback 到 SDK 内置 SAE 标准表.

未覆盖的部分:
- ❌ 不替换默认仪表 PNG (主仓 8 张通用主题已足够 GT-R 使用)
- ❌ 不重写告警阈值 (默认 9 条规则适用于通用涡轮车)

## 怎么用

把 `profile.cpp` 拷到你的 mods 仓 `src/app/`, 在 `main.cpp` 加 2 行:

```cpp
// src/app/main.cpp
#include <raceguard/backend.h>

namespace raceguard_examples::nissan_gtr_r35 {
    void registerProfile();
}

void setup() {
    raceguard::log::init(115200);
    raceguard::hal::platform::initHardware();

    raceguard_examples::nissan_gtr_r35::registerProfile();   // ← 加这行
    raceguard::backend::startAll();
}

void loop() { raceguard::backend::tick(); }
```

调用顺序: `registerProfile()` **必须**在 `backend::startAll()` 之前, 因为 startAll 内部
会调 `OBDManager::init()` → `PIDScheduler::addPID()`, 此时会按 profile bitmap 决定哪些 PID 入队.

烧固件 + 实车验证后, 你应该看到:
- 串口日志: `PID 0x10 被 profile bitmap 屏蔽, 跳过注册` (× 7 个不支持的 PID)
- 设置菜单 → DTC 子页若读到 P1217, 显示 `VR38 引擎过热保护 (水温/油温超限)` 而不是 `未知故障码`

## GT-R 厂家 DTC 列表参考

本 example 只放了 ~10 个最常见的码做演示. 完整列表需查:
- **R35 Service Manual** EC (Engine Control) section — 官方 ~200 个 P1xxx
- **Consult III+** 软件 DTC 索引 — Nissan 经销商工具
- 论坛: [nicoclub.com](https://nicoclub.com), [GT-R Life](https://gt-r-life.com)

社区贡献欢迎 PR 补全 `dtcDescribe()` switch 表.

## 已知限制

- GT-R 实车日志中, 0x66 (新型 MAF) 在低转 < 1500 RPM 时偶发返回 0xFF 异常值
  → 主仓 `pid_scheduler.cpp:isPlausible()` 已加 6V 下限过滤, 不会进 latestData.
- GT-R Mode 04 (Clear DTC) 实际清的是 ECM 内部码,
  TCM/BCM 码需要 Consult III+ 单独清.

## tested_with/

(预留目录, 后续放实车 OBD 日志样本和验证视频)
