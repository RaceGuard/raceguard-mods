#!/usr/bin/env bash
# fetch_core.sh — 从 GitHub Releases 拉预编译核心库 (.a + headers)
#
# 用法: ./scripts/fetch_core.sh [version]
#   不传 version: 拉最新 release
#   传 version (如 v0.1.0): 拉指定版本
#
# 输出:
#   lib/raceguard_core/libraceguard-core-<version>-<env>.a
#   lib/raceguard_core/include/raceguard/*.h
#
# 依赖: gh (GitHub CLI), 已通过 brew install gh + gh auth login 配置.
#
# ⚠️ v0.0.x: 核心库尚未发布到 Releases, 本脚本暂为占位.
#            v0.1.0 release 后才可用.

set -euo pipefail

REPO="RaceGuard/raceguard-mods"
LIB_DIR="$(cd "$(dirname "$0")/.." && pwd)/lib/raceguard_core"
VERSION="${1:-latest}"

mkdir -p "${LIB_DIR}"

# 检查 gh CLI
if ! command -v gh >/dev/null 2>&1; then
    echo "ERROR: 需要 gh (GitHub CLI). 安装: brew install gh"
    exit 1
fi

if ! gh auth status >/dev/null 2>&1; then
    echo "ERROR: gh 未登录. 执行: gh auth login"
    exit 1
fi

echo "===> Fetching raceguard_core from ${REPO} (release: ${VERSION})"

# 拉 .a 文件和 headers
# v0.2.0-dev.3+: latest 走 release list 取最新 (含 prerelease) — GitHub "latest" 标识只算
# 非 prerelease release, 开发期所有 release 都 prerelease 会让裸 gh release download 报 "not found".
if [[ "${VERSION}" == "latest" ]]; then
    LATEST_TAG=$(gh release list --repo "${REPO}" --limit 1 --json tagName -q '.[0].tagName' 2>/dev/null)
    if [[ -z "${LATEST_TAG}" ]]; then
        echo "ERROR: ${REPO} 没有任何 release"
        exit 1
    fi
    echo "===> Resolved 'latest' → ${LATEST_TAG} (含 prerelease)"
    VERSION="${LATEST_TAG}"
fi
DOWNLOAD_CMD="gh release download ${VERSION} --repo ${REPO}"

cd "${LIB_DIR}"
${DOWNLOAD_CMD} -p 'libraceguard-core-*.a' -p 'headers-*.tar.gz' -p 'checksums.sha256' --clobber 2>&1 || {
    echo
    echo "ERROR: 拉取失败. 可能原因:"
    echo "  1. v0.0.x 阶段核心库还没发布到 Releases (这是预期)"
    echo "  2. 网络问题或仓库权限问题"
    echo
    echo "v0.1.0 release 后本命令才会真正可用. 当前 v0.0.x 阶段只能"
    echo "查看源码和文档, 不能 build firmware."
    exit 1
}

# 校验
if [[ -f checksums.sha256 ]]; then
    echo "===> 校验 sha256"
    sha256sum -c checksums.sha256 || {
        echo "ERROR: sha256 校验失败, 文件可能损坏"
        exit 1
    }
fi

# 解压 headers
HEADERS_TAR=$(ls headers-*.tar.gz 2>/dev/null | head -1)
if [[ -n "${HEADERS_TAR}" ]]; then
    echo "===> 解压 ${HEADERS_TAR} → lib/raceguard_core/include/"
    mkdir -p include
    tar -xzf "${HEADERS_TAR}" -C include --strip-components=1
fi

echo
echo "===> 完成. lib/raceguard_core/ 内容:"
ls -lh "${LIB_DIR}/"
echo
echo "===> 现在可以编译:  pio run -e round-led-21"
