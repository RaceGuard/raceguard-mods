#!/usr/bin/env bash
# flash_all.sh — 一键烧 firmware + LittleFS (upload + uploadfs)
#
# 用法:
#   ./scripts/flash_all.sh                       # 自动检测端口
#   ./scripts/flash_all.sh /dev/cu.usbmodem14101 # 显式端口 (多板 / 端口异常时)
#
# 等价于:
#   pio run -e round-led-21 -t upload
#   pio run -e round-led-21 -t uploadfs
#
# 设计目的: 用户改完 mod 想完整烧一次, 一行命令省心 + 避免"只烧 firmware 忘 uploadfs"

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${REPO_ROOT}"

# pio 命令解析 (CI / 本地通用)
PIO_CMD="${PIO_CMD:-pio}"
if ! command -v "${PIO_CMD}" >/dev/null 2>&1; then
    for p in "$HOME/.platformio/penv/bin/pio" /usr/local/bin/pio /opt/homebrew/bin/pio; do
        if [[ -x "$p" ]]; then PIO_CMD="$p"; break; fi
    done
fi
if ! command -v "${PIO_CMD}" >/dev/null 2>&1; then
    echo "ERROR: pio 没装. brew install platformio 或 pip install -U platformio."
    exit 1
fi

# 端口
PORT_ARGS=""
if [[ $# -ge 1 ]]; then
    PORT_ARGS="--upload-port $1"
    echo "===> 使用端口: $1"
else
    echo "===> 自动检测端口 (Mac: /dev/cu.usb*, Linux: /dev/ttyUSB* 或 /dev/ttyACM*)"
fi

echo ""
echo "===> Step 1/2: 烧 firmware (pio run -t upload)"
"${PIO_CMD}" run -e round-led-21 -t upload ${PORT_ARGS}

echo ""
echo "===> Step 2/2: 烧 LittleFS (pio run -t uploadfs)"
"${PIO_CMD}" run -e round-led-21 -t uploadfs ${PORT_ARGS}

echo ""
echo "✅ 烧录完成. 看串口日志:"
echo "   ${PIO_CMD} device monitor -b 115200"
