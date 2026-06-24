# Getting Started — 第一次跑起来

> 目标读者: 第一次接触本项目的开发者. 完全没玩过 ESP32 / PIO 也能跟着走完.
>
> 全程预估时长: **新机器首次** ~45 min (含下载) | **已有环境** ~5 min.

---

## 0. 你需要什么

### 硬件 (有的话能烧机看实物效果, 没有也能 build firmware)

- **Waveshare ESP32-S3-Touch-LCD-2.1** (480×480 圆形屏, 这是当前唯一支持的硬件)
  - 淘宝 / Aliexpress 搜 "Waveshare ESP32-S3 2.1 inch round LCD"
  - 价格 ~¥250-300
- 一根 **USB-C 数据线** (不是仅充电的, 必须能传数据)
- (可选) ELM327 BLE OBD 适配器, 接车上看真实数据. ~¥50-100.
  没适配器的话烧完就跑 DEMO 模式 mock 数据动画.

### 软件 (本指南帮你装)

- Git
- Python 3.10+ (3.11 推荐)
- PlatformIO Core (CLI 版, 不需要 IDE)
- gh CLI (拉 release 用; 仓库 public 后**可选**, 直接 `curl` 也行)

### 操作系统

- ✅ **macOS** (Intel / Apple Silicon 都行) — 主力开发环境
- ✅ **Linux** (Ubuntu 22.04+ 测过)
- 🟡 **Windows** — WSL2 + Ubuntu 走 Linux 路径最稳; 原生 PowerShell 也能但要自己装 `gh`、`make`、串口驱动

---

## 1. 装环境 (一次性, 新机器才需要)

### macOS

```bash
# Homebrew (如果没装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# git / Python / gh CLI
brew install git python@3.11 gh

# PlatformIO Core (用独立 venv, 不污染系统 Python)
python3 -m venv ~/.platformio/penv
~/.platformio/penv/bin/pip install -U platformio

# 把 PIO 加 PATH (zsh 默认 shell)
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# 验证
pio --version    # 应该输出 PlatformIO Core, version 6.x.x
git --version
gh --version
```

### Linux (Ubuntu)

```bash
sudo apt update && sudo apt install -y git python3-venv python3-pip curl

# gh CLI (官方源)
curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | sudo dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg
sudo chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | sudo tee /etc/apt/sources.list.d/github-cli.list
sudo apt update && sudo apt install gh

# PIO
python3 -m venv ~/.platformio/penv
~/.platformio/penv/bin/pip install -U platformio
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# 串口权限 (烧机需要)
sudo usermod -a -G dialout $USER
# 然后 logout / login 一次让组生效
```

### Windows

**推荐用 WSL2 + Ubuntu**, 走 Linux 路径. 原生 Windows 走 PowerShell 也行但坑多.

WSL2 setup:
```powershell
# 管理员 PowerShell
wsl --install -d Ubuntu-22.04
# 重启, Ubuntu 第一次启动会让你设 user/pass
```

然后在 WSL Ubuntu 内执行上面 Linux 步骤. 烧机时 USB 设备需要用 `usbipd-win` attach 到 WSL — 见 [Microsoft 官方指南](https://learn.microsoft.com/en-us/windows/wsl/connect-usb).

### gh 登录 (仓库当前是 public, **可选**)

仅当你想用 `gh` CLI 操作仓库 (提 issue/PR 等) 才需要:

```bash
gh auth login
# 选 GitHub.com → HTTPS → Web browser → 复制 device code → 浏览器粘贴授权
```

仓库已 public 的话, `fetch_core.sh` 不需要 `gh` 登录, 用 `gh release download` 走匿名下载.

---

## 2. Clone + Build (每个开发者首次都做)

```bash
# 1. clone
git clone https://github.com/RaceGuard/raceguard-mods.git
cd raceguard-mods

# 2. 拉预编译核心库 (.a + headers, ~13MB, 从 GitHub Release 下载)
./scripts/fetch_core.sh v0.1.2-dev
#   这步会自动:
#     - 下载 libraceguard-core-0.1.2-dev-round-led-21.a (13MB 闭源核心)
#     - 下载 headers-0.1.2-dev.tar.gz (8KB 公开 API headers)
#     - 下载 checksums.sha256
#     - SHA-256 校验防损坏
#     - 解压 headers 到 lib/raceguard_core/include/

# 3. 编 firmware
pio run -e round-led-21
#   首次编会下载 platform-espressif32 + Arduino 框架 + 所有 lib 依赖
#   总下载 ~500MB, 总时长 ~5-10 min (后续编 ~30s 增量)
```

成功标志:
```
RAM:   [===       ]  27.7% (used 90908 bytes from 327680 bytes)
Flash: [========  ]  79.1% (used 4977161 bytes from 6291456 bytes)
========================= [SUCCESS] Took 43.29 seconds ===========
```

build 完后 firmware 在 `.pio/build/round-led-21/firmware.bin`. 没硬件就先到这里, **你已经成功了**.

---

## 3. 烧到 ESP32 (有硬件的话)

### 接线

1. USB-C 线把 Waveshare 板子接电脑
2. 板子背面默认有 USB-UART 桥, 不需要按住 BOOT/RST 那一套
3. macOS 会自动识别为 `/dev/cu.usbmodem*`; Linux `/dev/ttyACM0` 或 `/dev/ttyUSB0`

### 烧

```bash
pio run -e round-led-21 -t upload
```

如果失败提示找不到 port, 显式指定:
```bash
pio run -e round-led-21 -t upload --upload-port /dev/cu.usbmodem14101
# port 名字看你机器 ls /dev/cu.usb*
```

### 看串口日志

```bash
pio device monitor -b 115200
```

期望看到:
```
=================================================
RaceGuard core v0.1.2-dev
Platform: Waveshare-ESP32-S3-2.1
=================================================
[BootHeap] loop 启动前: ...
[LIC] Chip: AABBCCDDEEFF
[LIC] Not authorized, entering DEMO mode
[LED UI] DEMO 模式: 已弹激活提示卡
```

屏幕上同时:
1. 开机画面渐现 + 渐亮
2. 切到仪表页 (默认显示 COOLANT 表)
3. 3 秒后弹一次性 "DEMO MODE" 卡片 (你的 ChipId + Activate/Skip 两个按钮)
4. mock OBD 数据驱动指针缓慢摆动

**到这一步, 整个 SDK 就跑通了** 🎉

---

## 4. 开始二开

按你的目标走:

| 目标 | 看哪个 |
|------|--------|
| **适配我的车** (BMW / 本田 / 大众 ...) | [`ADD_CAR_PROFILE.md`](ADD_CAR_PROFILE.md) — 5 步 |
| **换仪表外观** | [`ADD_GAUGE_THEME.md`](ADD_GAUGE_THEME.md) — 5 步 |
| **写自定义 UI 卡片** | 看 `src/ui/common/card/png_gauge_card.cpp` 学着写, v0.2 出文档 |
| **接非 ESP32-S3 硬件** | 暂不支持. P4 长条屏 / EPD 见 [项目路线图](../README.md#路线图) |

---

## 5. 常见问题

### `pio: command not found`

- macOS / Linux: PATH 没加进去. `echo $PATH` 看有没有 `~/.platformio/penv/bin`.
- 临时解法: 用绝对路径 `~/.platformio/penv/bin/pio`

### `./scripts/fetch_core.sh: ERROR: 拉取失败`

- 网络问题: GitHub Releases 在国内有时慢, 可以挂代理或换个时段
- 仓库还是 private (历史遗留): 跑 `gh auth login` 后重试
- 版本号写错: 看 [releases 页面](https://github.com/RaceGuard/raceguard-mods/releases) 确认 tag 拼写

### `pio run` 卡在 "Installing platform" 很久

- 首次安装 `platform-espressif32` 约 200MB, 国内可能很慢
- 可以挂代理 (export `HTTP_PROXY` / `HTTPS_PROXY`) 或者用国内 PlatformIO 镜像
- 等它跑完一次, 后续编译不再下载

### 链接错误 `undefined reference to raceguard::...`

- 90% 是 `.a` 没拉对版本 / 文件名不匹配
- 检查 `ls lib/raceguard_core/*.a`, 应该有且只有一个 `libraceguard-core-<version>-round-led-21.a`
- 多个 `.a` 会让 `link_core.py` glob 匹配多个, 链接顺序不确定 → 删掉旧版

### 烧完串口没输出 / 屏幕黑

- USB 线是不是仅充电的 (没数据线)? 换一根
- 板子背面 BOOT 键按住, 同时点 RST, 强制进下载模式重烧一次
- 串口波特率: 必须 **115200** (`pio device monitor -b 115200`)

### 屏幕白屏 / 花屏

- 极可能是 PSRAM 时序问题. 详见主仓 `docs/troubleshooting/led-display-troubleshooting.md`
- 如果是新到的板子第一次烧, 试两三次启动. v0.1.2 已修复 BLE 初始化导致的纵向偏移问题

### `pio device monitor` 找不到 port

```bash
ls /dev/cu.usb*       # macOS
ls /dev/ttyUSB* /dev/ttyACM*    # Linux
```

如果没列出任何设备, 是 USB 没识别. 换线 / 换 USB 口 / 重插.

### Mac 上提示 "无法验证开发者" 之类

打开 系统设置 → 隐私与安全性 → 滚到底 → 允许 esptool / PlatformIO 之类的工具.

---

## 6. 下一步

- 看 [`examples/`](../examples/) 三个示例 mod, 跑跑看
- 加入社区: [GitHub Discussions](https://github.com/RaceGuard/raceguard-mods/discussions) (待启用)
- 报问题: [GitHub Issues](https://github.com/RaceGuard/raceguard-mods/issues)
- 想贡献代码看 [CONTRIBUTING.md](../CONTRIBUTING.md)

Have fun! 🏎️
