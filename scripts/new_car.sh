#!/usr/bin/env bash
# new_car.sh — 一行生成新车型 mod 框架 (从 bmw_template cp + sed namespace)
#
# 用法:
#   ./scripts/new_car.sh <品牌_车型_代号>
#
# 例:
#   ./scripts/new_car.sh honda_civic_fk7
#   ./scripts/new_car.sh toyota_supra_a90
#
# 产物:
#   examples/cars/<新车型>/profile.cpp   ← namespace 已改成你的车
#   examples/cars/<新车型>/README.md     ← 模板, 你填实车信息
#
# 下一步指引会自动打印.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [[ $# -ne 1 ]]; then
    echo "用法: $0 <品牌_车型_代号>"
    echo "例:   $0 honda_civic_fk7"
    echo ""
    echo "命名规则: 全小写, 下划线分隔, 含*年代代号*区分同型号不同年代"
    echo "  ✅ honda_civic_fk7    ✅ toyota_supra_a90    ✅ porsche_911_991"
    echo "  ❌ civic              ❌ Honda-Civic         (太泛/格式错)"
    exit 1
fi

NEW_CAR="$1"

# 校验命名
if [[ ! "${NEW_CAR}" =~ ^[a-z][a-z0-9_]*$ ]]; then
    echo "ERROR: 命名 '${NEW_CAR}' 不合法. 全小写字母+数字+下划线, 字母开头."
    exit 1
fi

SRC_DIR="${REPO_ROOT}/examples/cars/bmw_template"
DST_DIR="${REPO_ROOT}/examples/cars/${NEW_CAR}"

if [[ ! -d "${SRC_DIR}" ]]; then
    echo "ERROR: 模板目录不存在: ${SRC_DIR}"
    exit 1
fi

if [[ -e "${DST_DIR}" ]]; then
    echo "ERROR: 目标已存在: examples/cars/${NEW_CAR}/"
    echo "  改个名, 或者 rm -rf 后重跑"
    exit 1
fi

echo "===> 复制 bmw_template → examples/cars/${NEW_CAR}/"
cp -r "${SRC_DIR}" "${DST_DIR}"

echo "===> sed namespace bmw_template → ${NEW_CAR}"
# macOS sed -i 需要 '' 参数, GNU sed 不需要. perl -pi -e 跨平台.
find "${DST_DIR}" -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.md' \) \
    -exec perl -pi -e "s|bmw_template|${NEW_CAR}|g" {} \;

# 改 profile.cpp 内的车名 (默认 "BMW (Template)" → "<NEW_CAR>")
# 用户后续在 profile.cpp 内自己填准确车名
perl -pi -e "s|BMW \\(Template\\)|${NEW_CAR}|g" "${DST_DIR}/profile.cpp"

echo ""
echo "✅ 新车型 mod 已生成: examples/cars/${NEW_CAR}/"
echo ""
echo "===> 下一步:"
echo ""
echo "1. 改 examples/cars/${NEW_CAR}/profile.cpp:"
echo "   - 'name' 字段改成你车的全名 (e.g. \"Honda Civic FK7\")"
echo "   - kUnsupported[] 实车跑一段后填 NO DATA 的 PID"
echo "   - dtcDescribe() 加你抓到的厂家 P1xxx/U1xxx 故障码"
echo ""
echo "2. 改 src/app/active_mods.cpp 启用这个 mod:"
echo "   namespace raceguard_examples::${NEW_CAR} { void registerProfile(); }"
echo "   void register_active_mods() {"
echo "       raceguard_examples::${NEW_CAR}::registerProfile();"
echo "   }"
echo ""
echo "3. 改 platformio.ini build_src_filter 加这一行:"
echo "   build_src_filter = +<*> +<../examples/cars/${NEW_CAR}/>"
echo ""
echo "4. 烧机验证:"
echo "   pio run -e round-led-21 -t upload"
echo "   pio device monitor -b 115200"
echo ""
echo "5. 实车测试 ≥30 min 覆盖怠速+城市+高速, 详见 docs/ADD_CAR_PROFILE.md"
echo ""
echo "6. PR 提交: 用 branch feat/cars/${NEW_CAR}, commit prefix feat(cars):"
echo "   见 CONTRIBUTING.md branch / commit 约定段"
