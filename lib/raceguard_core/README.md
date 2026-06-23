# `lib/raceguard_core/` — 预编译核心库放置位置

本目录留作 `scripts/fetch_core.sh` 下载核心库的目标位置.

## 内容

`fetch_core.sh` 拉取后:

```
lib/raceguard_core/
├── libraceguard-core-v0.1.0-led.a    ← LED 圆屏核心库
├── libraceguard-core-v0.1.0-p4.a     ← P4 长条屏核心库 (后续)
├── headers-v0.1.0.tar.gz             ← 公开 header 打包
├── checksums.sha256
└── include/
    └── raceguard/                     ← 解压后的公开 header (与本仓 include/ 同步)
```

`.gitignore` 已忽略 `*.a / *.so / *.dylib / include/`, 不会进 git.

## 当前状态 (v0.0.x)

⚠️ **核心库暂未发布到 Releases**. 本目录现在是空的, 只有本 README.

v0.1.0 release 后, 用户执行:
```bash
./scripts/fetch_core.sh v0.1.0
```
即可下载核心库到此目录, 之后 `pio run` 能正常 link 出 firmware.

## 核心库内容

核心库以预编译形式分发, 帮你搞定所有硬件适配 + OBD 协议复杂度:

- HAL: DSI / RGB panel 时序 / DMA 协调 / PSRAM 二阶段启动
- OBD 协议栈: ELM327 BLE / CAN 直连 / PID 调度 / DTC 解析
- 告警引擎: 9 条规则 / P0-P3 四级
- SD 异步日志: LogWriter 多核协作
- IMU / NVS / 触摸等驱动

公开 API 在 `include/raceguard/*.h`, 通过 `raceguard::*` namespace 暴露.
你不用看核心库怎么实现的, 直接调 API 写上层逻辑.
