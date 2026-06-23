#!/usr/bin/env python3
"""
把 src/ui/led/assets/gauge_*_480.png 8 张打包成 LVGL 可用的 C 数组。
每张 PNG 输出为 src/ui/led/assets/gauges/<name>.c（PNG 原始字节 + lv_img_dsc_t）。
另外生成 gauge_assets.h 汇总 LV_IMG_DECLARE。

LVGL 用 cf=LV_IMG_CF_RAW_ALPHA + LV_USE_PNG=1 在运行时调 lodepng 解码 PNG，
解码结果由 LVGL image cache 自动管理。

用法：
  python pack_assets.py
"""
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
SRC_DIR = PROJECT_ROOT / "src" / "ui" / "led" / "assets"
OUT_DIR = SRC_DIR / "gauges"

GAUGES = [
    "coolant", "oil_temp", "rpm", "speed",
    "volts", "intake", "boost", "afr",
]


C_TEMPLATE = """\
// 自动生成 — 请勿手动编辑。源：src/ui/led/assets/{name}_480.png
// 生成脚本：tools/gauge_bakery/pack_assets.py
#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

static const LV_ATTRIBUTE_MEM_ALIGN uint8_t {sym}_map[] = {{
{bytes_body}
}};

const lv_img_dsc_t {sym} = {{
    .header.cf = LV_IMG_CF_RAW_ALPHA,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 480,
    .header.h = 480,
    .data_size = sizeof({sym}_map),
    .data = {sym}_map,
}};
"""


def bytes_to_c_array(data: bytes, indent: str = "    ", per_line: int = 16) -> str:
    out = []
    for i in range(0, len(data), per_line):
        chunk = data[i:i + per_line]
        out.append(indent + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    return "\n".join(out)


def pack_one(name: str) -> int:
    png_path = SRC_DIR / f"gauge_{name}_480.png"
    if not png_path.exists():
        raise SystemExit(f"找不到 {png_path}")
    data = png_path.read_bytes()
    sym = f"gauge_{name}_480"
    body = bytes_to_c_array(data)
    out_c = OUT_DIR / f"{sym}.c"
    out_c.write_text(C_TEMPLATE.format(name=name, sym=sym, bytes_body=body))
    print(f"  → {out_c.relative_to(PROJECT_ROOT)}  ({len(data) / 1024:.1f} KB)")
    return len(data)


def write_header() -> None:
    decls = "\n".join(f"LV_IMG_DECLARE(gauge_{name}_480);" for name in GAUGES)
    header = f"""\
// 自动生成 — 请勿手动编辑。
// 包含 8 张仪表底图 C 数组的前置声明。
// 实际数据在同目录下的 gauge_*_480.c 中。
#pragma once
#include "lvgl.h"

{decls}
"""
    (OUT_DIR / "gauge_assets.h").write_text(header)
    print(f"  → {(OUT_DIR / 'gauge_assets.h').relative_to(PROJECT_ROOT)}")


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    total = 0
    print(f"打包 {len(GAUGES)} 张 PNG 到 {OUT_DIR.relative_to(PROJECT_ROOT)}/：")
    for name in GAUGES:
        total += pack_one(name)
    write_header()
    print(f"\n总计：{total / 1024:.1f} KB ({total / (1024 * 1024):.2f} MB)")


if __name__ == "__main__":
    main()
