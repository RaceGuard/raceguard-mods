# RaceGuard 烧录指南 — 从零开始

> 目标读者: 完全不会编程, 只想买硬件 + 烧固件 + 用. 不需要安装 PlatformIO / Python / git.
> 全程预估: 第一次 ~30 分钟 (含硬件到货后), 熟练后 ~5 分钟.

---

## 1. 准备硬件

### 必买

| 物品 | 型号 | 预算 | 哪买 |
|------|------|------|------|
| 主板 | Waveshare ESP32-S3-Touch-LCD-2.1 | ¥250 | [淘宝官方店](https://item.taobao.com/item.htm?id=767434648083) / [Aliexpress](https://www.aliexpress.com/item/1005006054945275.html) |
| USB 数据线 | USB-A 或 USB-C 转 USB-C, **必须支持数据传输** | ¥10 | 不要用仅充电线 |

### 推荐买 (强烈, 解决持久化)

| 物品 | 规格 | 预算 | 说明 |
|------|------|------|------|
| microSD 卡 | 任意品牌 ≥1GB, FAT32 格式 | ¥10 | 激活码 + 日志 + 自定义 logo 持久化用; 没卡只能存 NVS, 擦了就丢 |

### 可选 (后期接车)

| 物品 | 型号 | 预算 | 说明 |
|------|------|------|------|
| OBD-II BLE 适配器 | ELM327 v1.5+ (蓝牙非 WiFi) | ¥50-100 | 接车 OBD 口读真实数据 |

---

## 2. 准备烧录工具

**最简单路径 — 浏览器烧录** (推荐, 不装任何软件):

需要 **Chrome** 或 **Edge** 浏览器 (Safari / Firefox 不支持 WebSerial API).

打开: <https://espressif.github.io/esptool-js/>

或备选: <https://web.esphome.io/> (也是 WebSerial)

---

## 3. 接线 + 烧固件

### 3.1 接线

1. USB-C 线一头插主板 (背面 USB 口), 另一头插电脑
2. 板子背面 LED 应该亮 (电源指示)
3. 屏幕可能黑屏 (没固件) 或显示厂家测试画面

### 3.2 进入烧录模式

板子背面有两个按钮:
- **BOOT** (有些板印 "9" 或 "IO9")
- **RST** (有些板印 "EN" 或 "Reset")

操作:
1. 按住 **BOOT** 不放
2. 短按一下 **RST**
3. 松开 **BOOT**

此时板子进入下载模式 (屏幕全黑).

### 3.3 浏览器烧录

打开 <https://espressif.github.io/esptool-js/>:

1. **Baud rate** 选 `921600` (快) 或 `115200` (稳)
2. 点 **Connect** → 弹窗选 `USB JTAG/serial debug unit` 或 `USB Single Serial` (Mac 显示 `/dev/cu.usbmodem*`, Windows 显示 `COMx`)
3. 连接成功显示芯片信息: `Chip is ESP32-S3 (revision v0.2)`
4. **Flash Address**: 输 `0x0`
5. **Choose File**: 选你下载的 `raceguard-firmware-v0.2.0-dev.3-led-21.bin` (16 MB)
6. 点 **Program** → 等 ~30-60 秒进度条跑完
7. 显示 `Hash of data verified` → 烧录成功

### 3.4 重启板子

按一下 **RST** (或拔插 USB).

屏幕应该出现:
- 开机画面 (渐亮)
- 几秒后切到仪表盘 (默认 COOLANT 表)
- 弹一次性 "DEMO MODE" 卡片 (你的 ChipId + Activate/Skip 按钮)

**到这里就成功了** 🎉

---

## 4. 命令行烧录 (开发者备选)

如果你用过 Python / 命令行, 可以装 esptool:

```bash
pip install esptool

# 找端口 (Mac/Linux)
ls /dev/cu.usb* /dev/ttyUSB* /dev/ttyACM* 2>/dev/null

# 烧 (替换端口)
esptool.py --chip esp32s3 --port /dev/cu.usbmodem14101 --baud 921600 \
    write_flash 0x0 raceguard-firmware-v0.2.0-dev.3-led-21.bin
```

---

## 5. 首次启动 + DEMO 模式

烧完默认进 **DEMO 模式**:
- UI 框架已激活, 仪表盘动画跑起来 (mock 数据驱动)
- OBD / SD 日志 / 告警等"全功能"未解锁

点屏幕底部 / 滑动手势进入 **设置菜单** → 点 **ACTIVATE**:
- 屏幕显示 WiFi AP 二维码 + 你的 Chip ID
- 手机连 WiFi `RaceGuard-Setup` (密码 `raceguard`)
- 浏览器自动跳激活页 (或手输 `http://192.168.4.1`)
- 输 **16 位激活码** (问卖家要 / 联系 raceguard.cn)
- 提交成功 → 屏幕自动重启

重启后看到 **ACTIVATED**, 全功能解锁:
- OBD 自动扫 BLE 适配器
- SD 日志记录
- 告警弹窗
- 主题切换

**激活码会同时写到 NVS + SD 卡 `/license.txt`**, 后续即使整片擦 flash, 只要 SD 卡在, 启动自动恢复, 不需要再激活.

---

## 6. 接车 + 看数据

1. 买 ELM327 BLE 适配器
2. 插车 OBD-II 口 (一般在方向盘下方)
3. 板子启动后自动扫附近 BLE 设备, 找到 ELM327 自动连
4. 仪表盘开始显示实时数据 (转速 / 速度 / 水温 等)

---

## 7. 常见问题

### Q: 浏览器烧录时点 Connect 没弹窗

- 你用的可能不是 Chrome/Edge (Safari/Firefox 不支持 WebSerial)
- 板子没进下载模式 (按 BOOT + 点 RST + 松 BOOT 重来)
- USB 线是充电线 (换数据线)

### Q: 烧完屏幕黑 / 没动静

- 检查烧到的 Address 是 `0x0` (不是别的)
- 文件名要是 `raceguard-firmware-v0.2.0-dev.3-led-21.bin` (16MB), 不是部分文件
- 重启时拔插一次 USB

### Q: 屏幕有内容但仪表盘没数据

- 默认 DEMO 模式, 仪表显示 mock 数据 (慢慢摆动) — 这是正常的
- 想看真实数据需要: 激活 + 插 ELM327 + 接车

### Q: 我没有激活码

- DIY 用户: GitHub 项目暂未对外发售, 关注 raceguard.cn 公告
- 没激活也能用 DEMO 模式看 UI 效果

### Q: 想自己改代码 / 加车型 / 换主题

- 见 [`GETTING_STARTED.md`](GETTING_STARTED.md) (开发者向, 需 PlatformIO)
- 见 [`ADD_CAR_PROFILE.md`](ADD_CAR_PROFILE.md) 自己适配车型
- 见 [`ADD_GAUGE_THEME.md`](ADD_GAUGE_THEME.md) 自己做主题

### Q: 我想升级新版本固件

每次新 release 这里有新的 `raceguard-firmware-v?.?.?-led-21.bin` 文件, 重复 Step 3 烧新版即可. 烧 firmware **不会清** SD 卡 (主题包 / 激活码备份在 SD 卡里), 也不会清 NVS (除非你显式 erase).

如果想彻底重置, 用 esptool:
```bash
esptool.py --chip esp32s3 erase_flash    # ⚠️ 核武器, 擦激活 + 主题 + 日志
```
然后重新烧固件 + 重新激活.

---

## 8. 反馈 / 报问题

- 浏览器烧录失败 / 屏幕异常 → [GitHub Issues](https://github.com/RaceGuard/raceguard-mods/issues)
- 想买激活码 / 商业合作 → raceguard.cn (邮箱待启用, 暂用 GitHub Issue 标 `business` label)

---

享受! 🏎️
