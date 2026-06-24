# 添加你的车型适配 — Step-by-step 指南

> 目标读者: ESP32 仪表用户, 想让 SDK 认识自己的车 (BMW / 本田 / 大众 / 国产新势力 等等).
> 前提: 已经能 `pio run -e round-led-21` build 出 firmware 并烧到 ESP32, DEMO 模式 UI 跑通.

## 5 步搞定 (~30 min 写代码 + ~1 小时实车测试)

### Step 1: 用 cookie-cutter 一行生成

```bash
./scripts/new_car.sh <品牌_车型_代号>
# 例: ./scripts/new_car.sh honda_civic_fk7
```

脚本自动: cp bmw_template + sed namespace 全替换 + 打印接下来 6 步指引.

(老式手工: `cp -r examples/cars/bmw_template examples/cars/<新车型>/` 也行, 但要手动 sed namespace, 容易漏)

命名规则: 全小写, 下划线分隔, 含 *年代代号* 区分同型号不同年代:
- ✅ `honda_civic_fk7` (10 代思域 17-21)
- ✅ `toyota_supra_a90` (5 代 supra)
- ❌ `civic` (太泛, 同车型不同代 ECU 可能不一样)

### Step 2: 改 `profile.cpp` 的 namespace 和车型名

```cpp
namespace raceguard_examples::honda_civic_fk7 {   // 改 namespace

const raceguard::car::Profile kProfile = {
    /* .name = */ "Honda Civic FK7 (10th gen)",   // 改车型名
    ...
};
```

### Step 3: 把车型 register 接到 main.cpp

```cpp
// src/app/main.cpp
namespace raceguard_examples::honda_civic_fk7 {
    void registerProfile();
}

void setup() {
    raceguard::log::init(115200);
    raceguard::hal::platform::initHardware();

    raceguard_examples::honda_civic_fk7::registerProfile();   // ← 加这行
    raceguard::backend::startAll();
}
```

把 `examples/cars/honda_civic_fk7/profile.cpp` 拷到 `src/app/` 或者用 `build_src_filter` 引用:

```ini
; platformio.ini 加一行
build_src_filter =
    +<*>
    +<../examples/cars/honda_civic_fk7/profile.cpp>
```

### Step 4: 实车跑 + 抓 PID 边界

烧固件, 接 ELM327 BLE 适配器到车上 OBD 口, 跑一段:

```bash
pio run -e round-led-21 -t upload
pio device monitor -b 115200
```

日志关键行:

```
[OBD] PID 0x10 request → NO DATA  ← 不支持
[OBD] PID 0x5C request → 100 (oil_temp)  ← 支持
[OBD] PID 0xFF request → NO DATA          ← 厂家保留, 不支持
```

把 NO DATA 的 PID 记下来, 回到 `profile.cpp` 加到 `kUnsupported[]`:

```cpp
constexpr uint8_t kUnsupported[] = {
    0x10,   // MAF — Honda Civic FK7 NO DATA (用 0x66 替代)
    0xFF,   // 厂家保留
    // ... 你抓到的其他 PID
};
```

重 build 烧机, 日志应该显示:

```
PID 0x10 被 profile bitmap 屏蔽, 跳过注册
```

### Step 5: 抓厂家 DTC 描述

如果车有故障灯 (CEL 亮), 进设置菜单 → DTC 子页, 看读到啥 P1xxx / U1xxx 码.
没有故障灯的话, 也可以用 OBD 工具 (Carista / Torque 等) 看历史 DTC.

把这些码的描述填到 `dtcDescribe()`:

```cpp
const char* dtcDescribe(uint16_t code) {
    uint8_t type = (code >> 14) & 0x03;
    uint16_t num = code & 0x3FFF;

    if (type == 0x00) {     // P
        switch (num) {
            case 0x1077: return "Honda VTC 油压控制系统";   // ← 你查到的
            case 0x1259: return "VTEC 切换电磁阀异常";
            default: break;
        }
    }
    return nullptr;   // fallback 到 SAE 标准表
}
```

厂家码描述查询渠道:
- BMW: INPA / ISTA-D 软件 DTC 索引
- Honda: HDS (Honda Diagnostic System) / 论坛 (8thcivic.com / civicx.com)
- Toyota: Techstream 软件
- 大众系: VCDS / ODIS 软件
- 国产新势力: 找品牌客户端 / 论坛 / 4S 店 DTC 表

## Step 6: 提交 PR

```bash
git checkout -b feat/cars/honda_civic_fk7
git add examples/cars/honda_civic_fk7/
git commit -m "feat(cars): add Honda Civic FK7 profile (实车验证 50min, 城市+高速)"
git push origin feat/cars/honda_civic_fk7
gh pr create --base main
```

PR description 模板:

```markdown
## 车型信息
- 品牌车型: Honda Civic FK7 (10th gen)
- 年款: 2017-2021
- 国别: 中国
- 引擎: 1.5T (L15B7)

## 测试覆盖
- [x] 怠速 10 min, RPM/水温/电压跳动正常
- [x] 城市行驶 20 min, 速度/油门/STFT 实时
- [x] 高速 20 min, 长 LTFT 稳定 ±3%
- [x] DTC 子页注入 P1259 → 显示 "VTEC 切换电磁阀异常"

## 已知限制
- 0x33 BARO 不支持 (本田 1.5T 用 MAP 代替)
- 厂家 DTC 描述只覆盖 ~10 个最常见, 完整列表 ~80 个

## 实车证据
- 视频: <screencast.mp4 链接>
- 串口日志: <log.txt 链接>
```

## 常见坑

### Q: bitmap 编码搞不清楚

bitmap 是 32 字节, 每 bit 对应一个 PID. MSB-first:
- `bitmap[0]` bit 7 = PID 0x00, bit 6 = PID 0x01, ..., bit 0 = PID 0x07
- `bitmap[1]`         = PID 0x08..0x0F
- ...
- `bitmap[31]`        = PID 0xF8..0xFF

模板用 `constexpr buildBitmap()` 函数生成, 你只需在 `kUnsupported[]` 数组里写 PID 号
(十六进制), 编译期自动算好 bitmap. 不需要手算字节.

### Q: registerProfile 不生效?

检查调用顺序: `registerProfile()` **必须在** `raceguard::backend::startAll()` 之前调.
否则 OBDManager init 时 profile 还没注册, addPID 看到的是 nullptr → 默认全开.

### Q: PID 0x10 显示 NO DATA, 但 GT-R 模板里说"用 0x66 替代", 我的车要不要换?

不用. SDK 内部 PID 调度会自动同时尝试 0x10 和 0x66. 你只需在 bitmap 关掉不支持的,
另一个会自动接管.

### Q: dtcDescribe 返回 nullptr 还是返回空字符串?

返回 `nullptr` — SDK 会自动 fallback 到内置 SAE 标准表 (86 条 P0xxx 全覆盖).
返回空字符串 `""` 会被当成"已处理"导致显示 "未知故障码".

### Q: 我能不能在 mod 里改告警阈值?

当前 v0.1.2 不行, 告警阈值是 `.a` 内固定的 (水温 >115°C / 油温 >135°C 等).
v0.2.x 会加 `raceguard::alert::overrideThreshold()` API. 当前需要的话, 提 issue 讨论.

## 参考

- 完整 GT-R 实例: [`examples/cars/nissan_gtr_r35/`](../examples/cars/nissan_gtr_r35/)
- BMW 起步模板: [`examples/cars/bmw_template/`](../examples/cars/bmw_template/)
- 公开 API 文档: [`include/raceguard/car.h`](../include/raceguard/car.h)
- PID 解析公式 / SAE J1979 参考: 主仓 `docs/reference/PID_REFERENCE.md` (待迁入本仓)

---

## 目录约定 / 命名规则

`examples/cars/<品牌_车型_代号>/` (全小写, 下划线分隔, 含*年代代号*区分同型号不同代 ECU)

```
✅ nissan_gtr_r35      (R35 一代 GT-R, 07-至今, 不同年份 ECU 差异)
✅ toyota_supra_a90    (5 代 supra, A90 平台)
✅ porsche_911_991     (991 platform, 12-19)
✅ volkswagen_golf_mk7 (Mk7 高尔夫)
❌ civic               (太泛, 不同代 ECU 不一样)
```

## 合并标准 (PR Review 时检查)

- ✅ 实车连续测试 ≥30 分钟, 覆盖怠速 + 城市 + 高速三种工况
- ✅ `kUnsupported[]` 标的 PID 实测真的返回 NO DATA (串口日志为证)
- ✅ 至少一个 P1xxx 厂家码被 `dtcDescribe()` 处理 (用 OBD 工具人为注入也行)
- ✅ README 列出: 测试车型 / 年份 / 国别 / 已知限制 / 视频或日志证据链接
- ✅ 代码风格遵循 SDK 模板 (constexpr buildBitmap / dtcDescribe switch / namespace 命名)
- ⚠️ 不要把厂家私有协议 (Mode 22 / UDS) 写进 example, 那些需要专门 license / 合规审查
- ⚠️ 不要 hardcode 激活码 / API key / 公司密钥等敏感信息
