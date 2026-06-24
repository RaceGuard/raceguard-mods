#!/usr/bin/env bash
# new_theme.sh — 一行生成新主题包框架 (从 dark_minimal cp + 改 manifest name)
#
# 用法:
#   ./scripts/new_theme.sh <风格名> [作者名]
#
# 例:
#   ./scripts/new_theme.sh cyberpunk_neon "Your Name"
#   ./scripts/new_theme.sh jdm_retro
#
# 产物:
#   data/themes/<新风格>/manifest.json   ← name/author 已改, 你填 version 和 description
#   data/themes/<新风格>/gauge_*.png × 8 ← 从 dark_minimal 拷的占位 PNG
#   examples/themes/<新风格>/README.md   ← 你填设计灵感 + 配色说明
#
# 替换 PNG 是你的设计工作 (Figma/PS), 详见 docs/ADD_GAUGE_THEME.md.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [[ $# -lt 1 ]]; then
    echo "用法: $0 <风格名> [作者名]"
    echo "例:   $0 cyberpunk_neon \"Your Name\""
    echo ""
    echo "命名规则: 全小写, 下划线分隔, 描述视觉感"
    echo "  ✅ cyberpunk_neon  ✅ jdm_retro  ✅ digital_modern"
    echo "  ❌ MyTheme         ❌ classic    (大小写错/太泛)"
    exit 1
fi

NEW_THEME="$1"
AUTHOR="${2:-Anonymous}"

# 校验命名
if [[ ! "${NEW_THEME}" =~ ^[a-z][a-z0-9_]*$ ]]; then
    echo "ERROR: 命名 '${NEW_THEME}' 不合法. 全小写字母+数字+下划线, 字母开头."
    exit 1
fi

SRC_DATA="${REPO_ROOT}/data/themes/dark_minimal"
DST_DATA="${REPO_ROOT}/data/themes/${NEW_THEME}"
SRC_EX="${REPO_ROOT}/examples/themes/dark_minimal"
DST_EX="${REPO_ROOT}/examples/themes/${NEW_THEME}"

if [[ ! -d "${SRC_DATA}" ]]; then
    echo "ERROR: 模板目录不存在: ${SRC_DATA}"
    exit 1
fi

if [[ -e "${DST_DATA}" || -e "${DST_EX}" ]]; then
    echo "ERROR: 目标已存在 (data/themes/${NEW_THEME}/ 或 examples/themes/${NEW_THEME}/)"
    echo "  改个名, 或者 rm -rf 后重跑"
    exit 1
fi

echo "===> 复制 dark_minimal → data/themes/${NEW_THEME}/ (含 8 张占位 PNG + manifest.json)"
cp -r "${SRC_DATA}" "${DST_DATA}"

echo "===> 复制 examples/themes/dark_minimal → examples/themes/${NEW_THEME}/"
cp -r "${SRC_EX}" "${DST_EX}"

echo "===> 改 manifest.json name + author"
# 用 python (mods 仓有 PIO Python, 一定有) 改 JSON, 比 sed 安全
/Users/unicell/.platformio/penv/bin/python3 -c "
import json, sys
p = '${DST_DATA}/manifest.json'
m = json.load(open(p))
m['name'] = '${NEW_THEME}'.replace('_', ' ').title()
m['author'] = '''${AUTHOR}'''
m['version'] = '0.1.0'
json.dump(m, open(p, 'w'), indent=2, ensure_ascii=False)
print(f'manifest: {p}')
" 2>/dev/null || python3 -c "
import json, sys
p = '${DST_DATA}/manifest.json'
m = json.load(open(p))
m['name'] = '${NEW_THEME}'.replace('_', ' ').title()
m['author'] = '''${AUTHOR}'''
m['version'] = '0.1.0'
json.dump(m, open(p, 'w'), indent=2, ensure_ascii=False)
print(f'manifest: {p}')
"

echo "===> sed README dark_minimal → ${NEW_THEME}"
find "${DST_EX}" -type f -name '*.md' \
    -exec perl -pi -e "s|dark_minimal|${NEW_THEME}|g" {} \;
find "${DST_EX}" -type f -name '*.md' \
    -exec perl -pi -e "s|Dark Minimal Demo|${NEW_THEME}|g" {} \;

echo ""
echo "✅ 新主题包已生成:"
echo "   data/themes/${NEW_THEME}/         (烧 LittleFS 的实际内容)"
echo "   examples/themes/${NEW_THEME}/     (README 文档)"
echo ""
echo "===> 下一步:"
echo ""
echo "1. 替换 8 张 PNG (data/themes/${NEW_THEME}/gauge_*.png):"
echo "   设计 480×480 RGB PNG, 用 Figma/PS, 文件名跟 manifest.gauges[].file 对齐."
echo "   详细设计要点 (车内可读 / WCAG AA 对比度 / 角度规范) 见 docs/ADD_GAUGE_THEME.md"
echo ""
echo "2. (推荐) 用烘焙工具从设计源 PNG 重新生成:"
echo "   ./tools/gauge_bakery/bake_all.py --output-fs ${NEW_THEME} \\"
echo "       --manifest-author '${AUTHOR}' --manifest-version '0.1.0'"
echo "   cp -r tools/gauge_bakery/output/themes/${NEW_THEME}/* data/themes/${NEW_THEME}/"
echo ""
echo "3. 烧 LittleFS:"
echo "   pio run -e round-led-21 -t uploadfs"
echo ""
echo "4. 代码切到这个主题 (一次性, 写 NVS 后重启):"
echo "   main.cpp 加: raceguard::ui::theme::select(\"${NEW_THEME}\"); ESP.restart();"
echo "   (设置菜单 GUI 入口 v0.2.1 加, 当前必须代码切)"
echo ""
echo "5. 实拍验证 preview-day.png + preview-night.png (放 examples/themes/${NEW_THEME}/)"
echo ""
echo "6. PR 提交: branch feat/themes/${NEW_THEME}, commit prefix feat(themes):"
echo "   见 CONTRIBUTING.md branch / commit 约定段"
