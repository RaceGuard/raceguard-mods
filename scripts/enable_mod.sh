#!/usr/bin/env bash
# enable_mod.sh — 一条命令启用一个 mod (改 3 处: active_mods.cpp 两块 + platformio.ini)
#
# 用法:
#   ./scripts/enable_mod.sh cars <mod_name>     # 启用一个车型 mod
#   ./scripts/enable_mod.sh disable             # 全 disable, 回 stock 通用 OBD-II
#   ./scripts/enable_mod.sh status              # 看当前 enable 了什么
#
# 例:
#   ./scripts/enable_mod.sh cars honda_civic_fk7
#   ./scripts/enable_mod.sh cars nissan_gtr_r35  # 切车 (自动 disable 上一个)
#   ./scripts/enable_mod.sh disable
#
# 说明:
#   - 车型 mod 一次只能 enable 一个. 切换车型直接重跑 enable_mod 即可, 不用先 disable
#   - 主题 mod (themes/) 不通过本脚本启用. v0.2.0+ 走 LittleFS:
#       1. ./scripts/new_theme.sh + 设计 PNG
#       2. pio run -t uploadfs
#       3. 从设备设置菜单切主题
#   - 改完跑 pio run -e round-led-21 -t upload 或 ./scripts/flash_all.sh 烧机

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ACTIVE_MODS="${REPO_ROOT}/src/app/active_mods.cpp"
PIO_INI="${REPO_ROOT}/platformio.ini"

if [[ $# -lt 1 ]]; then
    cat <<EOF
用法:
  $0 cars <mod_name>     启用车型 mod
  $0 disable             全 disable
  $0 status              看当前状态

例: $0 cars honda_civic_fk7
EOF
    exit 1
fi

CMD="$1"

# ============ helper ============

# 替换 active_mods.cpp 两个 sentinel 块的内容
# $1 = sentinel 名 (DECL 或 CALL), $2 = 新内容 (多行 OK, 空字符串 = 清空)
rewrite_sentinel() {
    local NAME="$1"
    local NEW_BODY="$2"
    local BEGIN="// >>> ENABLE_MOD:${NAME}_BEGIN"
    local END="// <<< ENABLE_MOD:${NAME}_END"

    if ! grep -qF "${BEGIN}" "${ACTIVE_MODS}"; then
        echo "ERROR: 在 ${ACTIVE_MODS} 找不到 sentinel: ${BEGIN}"
        echo "  active_mods.cpp 被手改过? 从 git 恢复: git checkout -- src/app/active_mods.cpp"
        exit 1
    fi

    # 把新 body 写临时文件, sed 用 'r' 命令插入
    local TMP_BODY
    TMP_BODY=$(mktemp)
    trap 'rm -f "${TMP_BODY}"' RETURN
    printf '%s' "${NEW_BODY}" > "${TMP_BODY}"

    # awk 替换 BEGIN..END 之间的内容 (跨平台, 比 sed -i 更稳).
    # 用 index() substring 匹配, 因为 sentinel 可能带前导缩进 (CALL 块在函数内有 4 空格).
    local TMP_OUT
    TMP_OUT=$(mktemp)
    awk -v begin="${BEGIN}" -v end="${END}" -v body_file="${TMP_BODY}" '
        index($0, begin) {
            print
            while ((getline line < body_file) > 0) print line
            close(body_file)
            in_block = 1
            next
        }
        index($0, end) {
            in_block = 0
            print
            next
        }
        !in_block { print }
    ' "${ACTIVE_MODS}" > "${TMP_OUT}"
    mv "${TMP_OUT}" "${ACTIVE_MODS}"
}

# 改 platformio.ini build_src_filter
# 移除所有 +<../examples/cars/*> 段, 可选加新的
# $1 = 新 mod 路径 (空 = 只 disable)
rewrite_pio_filter() {
    local NEW_PATH="${1:-}"
    local TMP
    TMP=$(mktemp)

    # 1. 删除所有 +<../examples/cars/*> 段 (含前导空格)
    # 2. 如果传了新路径, 在 build_src_filter 行末尾加上
    perl -pe '
        if (/^build_src_filter\s*=/) {
            s|\s*\+<\.\./examples/cars/[^>]+>||g;
        }
    ' "${PIO_INI}" > "${TMP}"

    if [[ -n "${NEW_PATH}" ]]; then
        perl -pi -e "
            if (/^build_src_filter\s*=/) {
                s|\$| +<${NEW_PATH}>|;
            }
        " "${TMP}"
    fi

    mv "${TMP}" "${PIO_INI}"
}

# 当前 enabled (从 active_mods.cpp DECL 块解析). grep 无匹配返 1 + pipefail
# 会让整个调用失败, 用 || true 兜底保证 disable 状态下 status 仍能跑.
current_enabled() {
    awk '/>>> ENABLE_MOD:DECL_BEGIN/,/<<< ENABLE_MOD:DECL_END/' "${ACTIVE_MODS}" \
        | { grep -oE 'raceguard_examples::[a-z0-9_]+' || true; } \
        | sort -u \
        | sed 's|raceguard_examples::||'
}

# ============ status ============

if [[ "${CMD}" == "status" ]]; then
    echo "===> 当前 enabled mod:"
    enabled=$(current_enabled)
    if [[ -z "${enabled}" ]]; then
        echo "  (无 — 跑 stock 通用 OBD-II)"
    else
        echo "${enabled}" | sed 's|^|  - |'
    fi
    echo ""
    echo "===> platformio.ini build_src_filter:"
    grep -E '^build_src_filter\s*=' "${PIO_INI}" | sed 's|^|  |'
    exit 0
fi

# ============ disable ============

if [[ "${CMD}" == "disable" ]]; then
    echo "===> 清空 active_mods.cpp + platformio.ini"
    rewrite_sentinel DECL ""
    rewrite_sentinel CALL ""
    rewrite_pio_filter ""
    echo "✅ 全 disable. 现在编出的 firmware 跑 stock 通用 OBD-II."
    exit 0
fi

# ============ enable ============

if [[ "${CMD}" != "cars" ]]; then
    echo "ERROR: 不支持的 category '${CMD}'. 当前只支持 'cars'."
    echo "  主题 mod 走 LittleFS, 跑 pio run -t uploadfs + 设置菜单切, 不需要 enable_mod."
    exit 1
fi

if [[ $# -lt 2 ]]; then
    echo "ERROR: 需要 mod 名"
    echo "  例: $0 cars honda_civic_fk7"
    echo ""
    echo "可选 mod:"
    ls "${REPO_ROOT}/examples/cars/" 2>/dev/null | sed 's|^|  - |'
    exit 1
fi

MOD_NAME="$2"
MOD_DIR="${REPO_ROOT}/examples/cars/${MOD_NAME}"

if [[ ! -d "${MOD_DIR}" ]]; then
    echo "ERROR: examples/cars/${MOD_NAME}/ 不存在"
    echo ""
    echo "现有 mod:"
    ls "${REPO_ROOT}/examples/cars/" 2>/dev/null | sed 's|^|  - |'
    echo ""
    echo "生成新 mod 框架: ./scripts/new_car.sh ${MOD_NAME}"
    exit 1
fi

# 检查 mod 是否有 registerProfile (基本健康性)
if ! grep -rq "registerProfile" "${MOD_DIR}" 2>/dev/null; then
    echo "ERROR: ${MOD_DIR} 内找不到 registerProfile 实现"
    echo "  检查 profile.cpp 是否完整 (从 bmw_template 复制时漏了?)"
    exit 1
fi

echo "===> 启用 cars/${MOD_NAME}"

DECL_BODY="namespace raceguard_examples::${MOD_NAME} { void registerProfile(); }"
CALL_BODY="    raceguard_examples::${MOD_NAME}::registerProfile();"

rewrite_sentinel DECL "${DECL_BODY}"
rewrite_sentinel CALL "${CALL_BODY}"
rewrite_pio_filter "../examples/cars/${MOD_NAME}/"

echo "✅ 已启用 cars/${MOD_NAME}"
echo ""
echo "改动:"
echo "  - src/app/active_mods.cpp: registerProfile decl + call"
echo "  - platformio.ini: build_src_filter += +<../examples/cars/${MOD_NAME}/>"
echo ""
echo "下一步:"
echo "  pio run -e round-led-21 -t upload     # 烧 firmware"
echo "  pio device monitor -b 115200          # 看启动日志"
echo ""
echo "切到另一辆车: ./scripts/enable_mod.sh cars <other_name>"
echo "回到 stock:   ./scripts/enable_mod.sh disable"
