#!/usr/bin/env python3
"""PNG → LVGL 9 C array (RGB888), 可选 #ifdef 守卫.

LVGL 8 也能用 (lv_image_dsc_t 由 lv_compat.h 别名映射成 lv_img_dsc_t).
头里写的字段 (.header.magic / .header.cf / .stride / .data_size / .data) 在
LVGL 8 和 9 的结构体里都有, 字面初始化器跨版本兼容.

用法:
    python3 tools/png_to_lvgl9.py <input.png> <var_name> [output.c] [--guard MACRO]

例:
    python3 tools/png_to_lvgl9.py gauge.png my_gauge_bg gauge.c \\
        --guard DISPLAY_TYPE_P4_BAR
"""

from PIL import Image
from pathlib import Path
import argparse
import sys

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="输入 PNG")
    parser.add_argument("var_name", help="C 变量名 (extern const lv_image_dsc_t)")
    parser.add_argument("output", nargs="?", help="输出 .c 路径 (默认同输入)")
    parser.add_argument("--guard", help="#ifdef 守卫宏 (如 DISPLAY_TYPE_P4_BAR)")
    args = parser.parse_args()

    input_png = Path(args.input)
    var_name = args.var_name
    output_c = Path(args.output) if args.output else input_png.with_suffix('.c')
    guard = args.guard
    output_c.parent.mkdir(parents=True, exist_ok=True)

    img = Image.open(input_png).convert("RGB")
    W, H = img.size
    print(f"输入: {input_png}  {W}×{H}")

    # RGB888 in LVGL 9 = BGR byte order (LVGL 8 同)
    pixels = img.load()
    data = bytearray(W * H * 3)
    idx = 0
    for y in range(H):
        for x in range(W):
            r, g, b = pixels[x, y]
            data[idx]     = b
            data[idx + 1] = g
            data[idx + 2] = r
            idx += 3

    # ── 生成 .h ──
    output_h = output_c.with_suffix('.h')
    h_lines = [
        f"// Auto-generated from {input_png.name} by tools/png_to_lvgl9.py — DO NOT EDIT",
        "#pragma once",
        '#include "../../lv_compat.h"',     # 拉入 LVGL 8/9 image 类型/宏别名
        "",
        "#ifdef __cplusplus",
        'extern "C" {',
        "#endif",
        "",
        f"extern const lv_image_dsc_t {var_name};",
        "",
        "#ifdef __cplusplus",
        "}",
        "#endif",
    ]
    output_h.write_text('\n'.join(h_lines) + '\n')

    # ── 生成 .c ──
    c_lines = []
    c_lines.append(f"// Auto-generated from {input_png.name} by tools/png_to_lvgl9.py — DO NOT EDIT")
    c_lines.append('#include "../../lv_compat.h"')

    if guard:
        c_lines.append('')
        c_lines.append(f"#if defined({guard})")

    c_lines.append('')
    c_lines.append('#ifndef LV_ATTRIBUTE_MEM_ALIGN')
    c_lines.append('#define LV_ATTRIBUTE_MEM_ALIGN')
    c_lines.append('#endif')
    c_lines.append('')
    # 像素数据
    c_lines.append('static const LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_MEM_ALIGN')
    c_lines.append(f'uint8_t {var_name}_data[] = {{')
    chunk_size = 16
    for i in range(0, len(data), chunk_size):
        chunk = data[i:i + chunk_size]
        hex_str = ', '.join(f'0x{b:02X}' for b in chunk)
        c_lines.append(f'    {hex_str},')
    c_lines.append('};')
    c_lines.append('')

    # image dsc — LVGL 9 / 8 双兼容字面初始化
    c_lines.append('#if LVGL_VERSION_MAJOR >= 9')
    c_lines.append(f'const lv_image_dsc_t {var_name} = {{')
    c_lines.append(f'    .header = {{')
    c_lines.append(f'        .magic   = LV_IMAGE_HEADER_MAGIC,')
    c_lines.append(f'        .cf      = LV_COLOR_FORMAT_RGB888,')
    c_lines.append(f'        .flags   = 0,')
    c_lines.append(f'        .w       = {W},')
    c_lines.append(f'        .h       = {H},')
    c_lines.append(f'        .stride  = {W * 3},')
    c_lines.append(f'    }},')
    c_lines.append(f'    .data_size = {len(data)},')
    c_lines.append(f'    .data      = {var_name}_data,')
    c_lines.append(f'}};')
    c_lines.append('#else')
    # LVGL 8: 字段顺序略不同, 用同名宏 + 兼容字段
    c_lines.append(f'const lv_img_dsc_t {var_name} = {{')
    c_lines.append(f'    .header = {{')
    c_lines.append(f'        .cf       = LV_IMG_CF_TRUE_COLOR,')
    c_lines.append(f'        .always_zero = 0,')
    c_lines.append(f'        .reserved = 0,')
    c_lines.append(f'        .w        = {W},')
    c_lines.append(f'        .h        = {H},')
    c_lines.append(f'    }},')
    c_lines.append(f'    .data_size = {len(data)},')
    c_lines.append(f'    .data      = {var_name}_data,')
    c_lines.append(f'}};')
    c_lines.append('#endif')
    c_lines.append('')

    if guard:
        c_lines.append(f"#endif  // {guard}")

    output_c.write_text('\n'.join(c_lines) + '\n')

    size_kb = len(data) / 1024.0
    print(f"输出: {output_c}  ({size_kb:.1f} KB pixel data)" +
          (f"  guard={guard}" if guard else ""))
    print(f"     {output_h}")

if __name__ == "__main__":
    main()
