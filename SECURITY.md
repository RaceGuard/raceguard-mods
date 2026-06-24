# Security Policy

## Supported Versions

| 版本 | 安全更新 |
|------|---------|
| v0.1.x | ✅ active development |
| < v0.1 | ❌ not supported (pre-release, use latest) |

## 报告漏洞

如果你发现 **安全漏洞**, 请 **不要** 直接开 public GitHub Issue. 请通过以下渠道私下报告:

- **GitHub Security Advisory**: https://github.com/RaceGuard/raceguard-mods/security/advisories/new
  (推荐, 内置加密 + tracking)
- **Email**: security@raceguard.cn (TODO: 邮箱启用前临时用 GitHub Advisory)

我们承诺:
- 48 小时内确认收到
- 7 天内给出修复时间表 / 风险评估
- 修复发布后, 在 GitHub Security Advisory 致谢报告者 (除非你要求匿名)

## 范围

### In scope (本仓代码)

- `src/` (UI 框架, 开源代码)
- `examples/` (车型 / 主题 mods)
- `include/raceguard/` (公开 API header)
- `tools/` (烘焙工具)
- `scripts/` (fetch_core 等)
- 本仓 firmware 行为问题 (link 出的二进制崩溃 / 异常)

### Out of scope (闭源核心库)

- `libraceguard-core-*.a` 内部实现 — 这部分是 proprietary, 报漏洞请直接联系
  RaceGuard 维护团队 (同上邮箱), 我们会在 .a 下次发版时修复.
- 反编译 / 静态分析 `.a` 的"发现" — 违反 [NOTICE](NOTICE) 条款,
  不会被受理为漏洞.

## 已知风险 (用户应知)

| 风险 | 缓解 | 状态 |
|------|------|------|
| OBD ELM327 BLE 通信无加密 | 蓝牙物理距离限制 (~10m), 攻击窗口窄 | 行业通病 |
| WiFi AP 激活页 (192.168.4.1) 无 TLS | 仅在激活短期开启 (~3 min), 用 HMAC-SHA256 校验激活码 | 接受 |
| 烘焙工具 (`bake.py`) 接受用户 PNG, 未做严格校验 | 工具仅本地跑, 不联网 | 接受 |
| 用户车型 mod 写错 PID 导致写错 ECU 数据 | SDK 只发 Mode 01 (读) / Mode 04 (清 DTC), **不发 Mode 2E (写)**, 物理上隔离写操作 | 设计层缓解 |

## 漏洞披露时间表

- **T+0**: 报告收到 (确认收到 48h 内)
- **T+7d**: 风险评估完成 + 修复 ETA 通告
- **T+ETA**: 修复 release, GitHub Security Advisory 公告
- **T+ETA+90d**: 详细技术细节公开 (CVE 申请等)

低危漏洞可缩短 90d 等待期, 高危漏洞可延长 (通报 affected user 后再公开).
