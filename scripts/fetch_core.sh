#!/usr/bin/env bash
# fetch_core.sh — 从 GitHub Releases 拉预编译核心库 (.a + SDK bundle)
#
# 用法: ./scripts/fetch_core.sh [version]
#   不传 version: 拉最新 release
#   传 version (如 v0.1.0): 拉指定版本
#
# 输出:
#   lib/raceguard_core/libraceguard-core-<version>-<env>.a
#   lib/raceguard_core/include/raceguard/*.h     (SDK headers)
#   lib/raceguard_core/lv_conf.h                 (LVGL build 配置, 跟 .a 强 ABI 一致)
#
# 依赖: gh (GitHub CLI), 已通过 brew install gh + gh auth login 配置.
#
# 历史: v0.2.0-dev.6 起 tarball 名 headers-*.tar.gz → sdk-bundle-*.tar.gz,
#       内含 lv_conf.h. 拉更老 release 需手动: gh release download <ver> -p 'headers-*.tar.gz'

set -euo pipefail

REPO="RaceGuard/raceguard-mods"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LIB_DIR="${REPO_ROOT}/lib/raceguard_core"

# Version 解析优先级 (v0.2.0-dev.3+):
#   1. 命令行参数 ./scripts/fetch_core.sh <version>  (显式最优)
#   2. CORE_VERSION 文件 (本仓 pin 的版本, 维护者升级时改)
#   3. 'latest' fallback (无 pin 时拉 GitHub 最新 release, 含 prerelease)
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
${DOWNLOAD_CMD} -p 'libraceguard-core-*.a' -p 'sdk-bundle-*.tar.gz' -p 'checksums.sha256' --clobber 2>&1 || {
    echo
    echo "ERROR: 拉取失败. 可能原因:"
    echo "  1. 指定版本不存在或还没发布到 Releases"
    echo "  2. 网络问题或仓库权限问题"
    echo "  3. v0.2.0-dev.5 及更早版本 tarball 名是 headers-*.tar.gz (本脚本只认 sdk-bundle-*),"
    echo "     如需拉老版本: gh release download <ver> -p 'headers-*.tar.gz' -p 'libraceguard-core-*.a'"
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

# 解压 SDK bundle (含 include/raceguard/ + lv_conf.h)
# tarball 内布局: sdk-bundle-<ver>/{include/raceguard/*.h, lv_conf.h}
# --strip-components=1 把顶级 sdk-bundle-<ver>/ 削掉, 内容直接落到 LIB_DIR
BUNDLE_TAR=$(ls sdk-bundle-*.tar.gz 2>/dev/null | head -1)
if [[ -n "${BUNDLE_TAR}" ]]; then
    echo "===> 解压 ${BUNDLE_TAR} → ${LIB_DIR}/{include/raceguard/, lv_conf.h}"
    # 先清旧 include (defensive: 老版本 manifest 改了 header 列表时避免残留)
    rm -rf include
    tar -xzf "${BUNDLE_TAR}" --strip-components=1
fi

echo
echo "===> 完成. lib/raceguard_core/ 内容:"
ls -lh "${LIB_DIR}/"
echo
echo "===> 现在可以编译:  pio run -e round-led-21"
