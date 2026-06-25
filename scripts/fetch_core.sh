#!/usr/bin/env bash
# fetch_core.sh — 从 GitHub Releases 拉预编译核心库 (.a + SDK bundle)
#
# 用法: ./scripts/fetch_core.sh [version]
#   不传 version: 读 CORE_VERSION 文件; 没 CORE_VERSION 则拉最新 release (含 prerelease)
#   传 version (如 v0.2.0-dev.6): 拉指定版本
#
# 输出:
#   lib/raceguard_core/libraceguard-core-<version>-<env>.a
#   lib/raceguard_core/include/raceguard/*.h     (SDK headers)
#   lib/raceguard_core/lv_conf.h                 (LVGL build 配置, 跟 .a 强 ABI 一致)
#
# 依赖: curl + sha256sum + tar (Mac/Linux 系统自带). v0.2.0-dev.7 起去掉 gh CLI 依赖,
#       仓库 public 后直接走匿名 HTTPS, 不用 brew install gh / gh auth login.
#
# 历史: v0.2.0-dev.6 起 tarball 名 headers-*.tar.gz → sdk-bundle-*.tar.gz, 内含 lv_conf.h.
#       老版本 (≤ dev.5) 本脚本不支持, 需手拉 (URL 模式照下面 BASE_URL 替换 asset 名即可).

set -euo pipefail

REPO="RaceGuard/raceguard-mods"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIB_DIR="${REPO_ROOT}/lib/raceguard_core"
ENV_NAME="round-led-21"     # 当前唯一支持的 PIO env. 加 P4 时改成 cmd arg

# ============ 依赖检查 ============

for cmd in curl tar sha256sum; do
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        # macOS 老版本 sha256sum 名 shasum, 这里要求 sha256sum (brew install coreutils 提供, 或 alias)
        if [[ "${cmd}" == "sha256sum" ]] && command -v shasum >/dev/null 2>&1; then
            sha256sum() { shasum -a 256 "$@"; }
            export -f sha256sum
            continue
        fi
        echo "ERROR: 缺命令 '${cmd}'."
        case "${cmd}" in
            curl)       echo "  装: brew install curl  /  apt install curl" ;;
            tar)        echo "  装: 系统应该自带, 检查 PATH" ;;
            sha256sum)  echo "  装: brew install coreutils  (macOS) / 通常 Linux 自带" ;;
        esac
        exit 1
    fi
done

# ============ Version 解析 ============
# 优先级 (v0.2.0-dev.3+):
#   1. 命令行参数 (显式最优)
#   2. CORE_VERSION 文件 (本仓 pin 的版本)
#   3. 'latest' (无 pin 时拉 GitHub 最新, 含 prerelease)
# 设计目的: user fork 后跑 ./scripts/fetch_core.sh 总是拉到跟代码兼容的 .a, 不会因
# 主仓发新 release 突然踩到不兼容变更.

if [[ $# -ge 1 ]]; then
    VERSION="$1"
elif [[ -f "${REPO_ROOT}/CORE_VERSION" ]]; then
    VERSION=$(cat "${REPO_ROOT}/CORE_VERSION" | tr -d '[:space:]')
    echo "===> Using pinned version from CORE_VERSION: ${VERSION}"
else
    VERSION="latest"
fi

# 'latest' 走 GitHub API 取最新 tag (含 prerelease — gh "latest" 标识跳 prerelease, dev 期不能用)
if [[ "${VERSION}" == "latest" ]]; then
    API_URL="https://api.github.com/repos/${REPO}/releases"
    echo "===> Resolving 'latest' via ${API_URL}"
    LATEST_TAG=$(curl -fsSL "${API_URL}" \
        | grep -m1 '"tag_name"' \
        | sed -E 's/.*"tag_name":[[:space:]]*"([^"]+)".*/\1/')
    if [[ -z "${LATEST_TAG}" ]]; then
        echo "ERROR: 取不到最新 tag. ${REPO} 没 release? API 限流 (60/hour/IP)?"
        exit 1
    fi
    echo "===> Resolved → ${LATEST_TAG} (含 prerelease)"
    VERSION="${LATEST_TAG}"
fi

# Asset 文件名不带 'v' 前缀 (tag 是 v0.2.0-dev.6, asset 是 libraceguard-core-0.2.0-dev.6-...)
ASSET_VER="${VERSION#v}"
BASE_URL="https://github.com/${REPO}/releases/download/${VERSION}"

A_FILE="libraceguard-core-${ASSET_VER}-${ENV_NAME}.a"
BUNDLE_TAR="sdk-bundle-${ASSET_VER}.tar.gz"
CHECKSUMS="checksums.sha256"

mkdir -p "${LIB_DIR}"
cd "${LIB_DIR}"

# ============ 下载 ============

echo "===> Fetching from ${BASE_URL}"
# 交互 TTY 用 -#(hash 进度); 非交互 (CI / pipe / nohup) 用 -sS 静默 (避免 progress 刷 60KB log).
# --retry 3 兜底 SSL 偶发 (本地 LibreSSL 跟 GitHub 偶尔 SSL_ERROR_SYSCALL, 重试就通).
COMMON_OPTS=(--retry 3 --retry-delay 2 --connect-timeout 15)
if [[ -t 1 ]]; then
    CURL_OPTS=(-fL -# "${COMMON_OPTS[@]}")
else
    CURL_OPTS=(-fsSL "${COMMON_OPTS[@]}")
fi
for asset in "${A_FILE}" "${BUNDLE_TAR}" "${CHECKSUMS}"; do
    echo "  ↓ ${asset}"
    if ! curl "${CURL_OPTS[@]}" -o "${asset}" "${BASE_URL}/${asset}"; then
        echo
        echo "ERROR: 下载 ${asset} 失败. 可能原因:"
        echo "  1. 版本 ${VERSION} 不存在 (查 https://github.com/${REPO}/releases)"
        echo "  2. 该版本没这个 asset (老版本 ≤ dev.5 没 sdk-bundle, 本脚本只支持 dev.6+)"
        echo "  3. 网络 / GitHub 暂时不可达"
        exit 1
    fi
done

# ============ 校验 ============

if [[ -f "${CHECKSUMS}" ]]; then
    echo "===> 校验 sha256"
    sha256sum -c "${CHECKSUMS}" || {
        echo "ERROR: sha256 校验失败, 文件可能损坏. 重跑 fetch_core 一次."
        exit 1
    }
fi

# ============ 解 SDK bundle ============
# tarball 内布局: sdk-bundle-<ver>/{include/raceguard/*.h, lv_conf.h}
# --strip-components=1 把顶级 sdk-bundle-<ver>/ 削掉, 内容直接落到 LIB_DIR
echo "===> 解压 ${BUNDLE_TAR} → ${LIB_DIR}/{include/raceguard/, lv_conf.h}"
# 先清旧 include (defensive: 老版本 manifest 改 header 列表时避免残留)
rm -rf include
tar -xzf "${BUNDLE_TAR}" --strip-components=1

echo
echo "===> 完成. lib/raceguard_core/ 内容:"
ls -lh "${LIB_DIR}/"

# 打印 BUILD_INFO (核心库工具链版本, 报 issue 时让维护者快速 triage ABI 问题)
if [[ -f "${LIB_DIR}/BUILD_INFO.txt" ]]; then
    echo
    echo "===> Build info (link 失败时把这段附进 issue):"
    sed 's/^/  /' "${LIB_DIR}/BUILD_INFO.txt"
fi

echo
echo "===> 现在可以编译:  pio run -e ${ENV_NAME}"
