# Contributing to RaceGuard Mods

欢迎贡献! 本项目正处于 **v0.1.x 早期** 阶段, API 可能还在小幅调整,
但车型适配 / 主题包 这类纯应用层 PR 已经欢迎合入.

## 你能贡献什么

| 类型 | 路径 | 难度 | 价值 |
|------|------|------|------|
| 车型适配 (PID bitmap + DTC) | `examples/cars/<品牌_车型>/` | 入门 | ⭐⭐⭐ |
| 仪表主题 (PNG + 配置) | `examples/themes/<风格名>/` | 入门 | ⭐⭐⭐ |
| UI 框架改进 (PngGaugeCard 子类等) | `src/ui/common/` | 中级 | ⭐⭐ |
| 烘焙工具增强 | `tools/gauge_bakery/` | 中级 | ⭐ |
| 文档 / 教程 / 翻译 | `docs/` 或 README | 入门 | ⭐⭐ |

## 提交流程

1. **Fork 本仓库** 到你的账号
2. **创建分支** (命名见下方 ↓ Branch / Commit 约定)
3. **写代码 + 实测验证** (详见下文 ↓ 验证要求)
4. **Commit** — 中英文都行, 第一行简短描述, 见下方 commit 约定
5. **Push + PR** 到 `RaceGuard/raceguard-mods` 的 `main` 分支. CI 会自动跑 build,
   失败 (绿勾变红 X) 的 PR 不能 merge.
6. PR description 必须包含:
   - 实车 / 实设备验证证据 (照片 / 日志 / 视频片段)
   - 已知限制 / 未覆盖的工况

## Branch / Commit 约定

### Branch 命名 (按 PR 类型)

```
feat/cars/<品牌_车型>      ← 新车型 mod  (e.g. feat/cars/honda_civic_fk7)
feat/themes/<风格名>       ← 新主题包    (e.g. feat/themes/cyberpunk_neon)
feat/sdk/<feature>         ← SDK 新功能 (改 src/ui/common/ 等公共层)
fix/<scope>/<short>        ← bug fix    (e.g. fix/theme/path-prefix)
docs/<topic>               ← 纯文档     (e.g. docs/add-car-profile)
chore/<task>               ← 维护任务   (e.g. chore/bump-deps)
```

### Commit message 约定 (Conventional Commits)

第一行格式: `<type>(<scope>): <短描述>`

- type: `feat` / `fix` / `docs` / `chore` / `refactor` / `test`
- scope: 跟 branch 的二级一致 (`cars` / `themes` / `sdk` / `tools`...)

例:
```
feat(cars): add Honda Civic FK7 profile (实车验证 50 min)
fix(theme): manifest path 双前缀 bug (修 LittleFS.open 误带 /littlefs/)
docs(getting-started): 加 erase 最佳实践段
```

第一行 ≤ 72 字符. body (可选) 解释 *为什么* 改, 不解释 *做什么* (diff 已说明).

### Merge 策略

维护者合并 PR 时默认用 **Squash merge**, 把分支多个 commit 压成一个进 main:
- main 历史干净 (1 PR = 1 commit, 跟 release notes 一致)
- 你 fork 上的细节 commit (typo fix / WIP) 不污染主仓
- Squash 后的 commit message 用你 PR title (跟 branch 命名一致就很自然)

特殊情况 (大 refactor 想保留每步细节): PR description 注明 "请用 Rebase merge",
维护者 case by case 决定.

## 验证要求

### 车型 mod (`examples/cars/`)

- ✅ 实车连续测试 ≥30 分钟, 覆盖 **怠速 + 城市行驶 + 高速** 三种工况
- ✅ 串口日志显示 `kUnsupported[]` 标的 PID 真的返回 NO DATA
- ✅ 至少一个 P1xxx 厂家码被你的 `dtcDescribe()` 处理 (用 OBD 工具人为注入也行)
- ✅ README 列出: 测试车型 / 年份 / 国别 / 已知限制
- ⚠️ 不要把厂家私有协议 (Mode 22 / UDS) 写进 example,
  那些需要专门 license / 合规审查

### 主题 mod (`examples/themes/`)

- ✅ 设备实拍效果 (preview.png + preview-dark.png 日间 / 夜间)
- ✅ 480×480 圆屏可读性: 主数字 ≥56px, 副数字 ≥24px
- ✅ 配色对比度 WCAG AA (至少 4.5:1, 用 https://contrast-ratio.com/ 校验)
- ❌ **不要使用第三方品牌商标** (无真实 Ferrari / Porsche / BMW 等 logo)
- ❌ 不要照搬其他商业产品的 UI (法律风险)

### UI 框架改动 (`src/ui/common/`)

- ✅ LVGL 8 / 9 双兼容 (通过 `LVGL_VERSION_MAJOR` 守卫)
- ✅ Native test 通过: `pio test -e native`
- ✅ 至少一个 env 烧机验证: `pio run -e round-led-21 -t upload`

## Code Style

- C++17, 4 空格缩进, 行宽建议 ≤100
- 中文注释 OK, 但 API 文档 (header 内) 用英文 (社区可读)
- 函数命名 `camelCase`, 类型 `PascalCase`, 常量 `kCamelCase` 或 `SCREAMING_SNAKE`
- 不要写无意义注释 ("// 初始化变量") — 注释解释 *为什么*, 不解释 *做什么*

## 双仓 sync 提示 (维护者用, 贡献者忽略)

部分文件 (UI 框架 / 公开 header / 烘焙工具) 是从主仓 `GTR-BlackBox` rsync 同步过来的.
PR 改这些文件会被维护者人工挑回主仓 + 下次 release 时同步.
**车型 / 主题 examples 是 mods 仓原生的, 直接合入即可**.

## Review 时长

- 车型 / 主题 example: 1 周内 first response
- UI 框架 / 工具: 2 周内 first response (需要在主仓 cherry-pick + 双仓 build 验证)

## 行为准则

遵循 [Contributor Covenant 2.1](https://www.contributor-covenant.org/version/2/1/code_of_conduct/).
简版: 友善, 专业, 对事不对人. 严重违反 → 仓库 ban.

## 联系

- GitHub Issues: https://github.com/RaceGuard/raceguard-mods/issues
- 私下报告安全问题: 见 [SECURITY.md](SECURITY.md)
