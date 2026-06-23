#!/usr/bin/env python3
"""
通过"复制圆环完整切片 → 旋转 → 贴到缺口位置"修补圆形仪表的环上缺口。
比手画弧线更自然——保留原圆环的所有细节（粗细、光晕、颜色渐变）。

用法：
  python seal_by_copy.py src.png dst.png \
      --gap 13:75 --src 200:262 \
      --gap 125:134 --src 200:209 \
      --r-inner 985 --r-outer 1035

每个 --gap 后面必须紧跟一个 --src 提供来源弧（长度应 ≥ gap）。
角度均用钟表坐标：12 点 = 0°，顺时针为正。
"""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops, ImageFilter


def make_arc_band_mask(cx: int, cy: int, r_inner: int, r_outer: int,
                       clock_start: int, clock_end: int, size: tuple[int, int]) -> Image.Image:
    """环带切片 mask（钟表坐标→PIL 坐标）"""
    pil_start = (clock_start - 90) % 360
    pil_end = (clock_end - 90) % 360
    pie = Image.new("L", size, 0)
    ImageDraw.Draw(pie).pieslice(
        (cx - r_outer, cy - r_outer, cx + r_outer, cy + r_outer),
        start=pil_start, end=pil_end, fill=255,
    )
    hole = Image.new("L", size, 0)
    ImageDraw.Draw(hole).ellipse(
        (cx - r_inner, cy - r_inner, cx + r_inner, cy + r_inner),
        fill=255,
    )
    return ImageChops.subtract(pie, hole)


def seal_gap(im: Image.Image, cx: int, cy: int,
             gap_start: int, gap_end: int,
             src_start: int, src_end: int,
             r_inner: int, r_outer: int,
             feather: int) -> Image.Image:
    """从 src 弧旋转粘贴到 gap 位置（边缘羽化避免硬接缝）。"""
    w, h = im.size
    src_center = (src_start + src_end) / 2
    gap_center = (gap_start + gap_end) / 2
    cw_deg = (gap_center - src_center) % 360
    rotated = im.rotate(-cw_deg, center=(cx, cy), resample=Image.BICUBIC)
    # 用比目标稍宽的源弧覆盖，再羽化 mask → 边缘平滑混合
    pad = max(feather // 2, 4)
    mask = make_arc_band_mask(cx, cy, r_inner, r_outer,
                              gap_start - pad, gap_end + pad, (w, h))
    if feather > 0:
        mask = mask.filter(ImageFilter.GaussianBlur(radius=feather))
    im.paste(rotated, (0, 0), mask)
    print(f"sealed gap {gap_start}°→{gap_end}° using src {src_start}°→{src_end}° "
          f"(rotated cw {cw_deg:.0f}°, band r={r_inner}-{r_outer}, feather={feather})")
    return im


def main(args: argparse.Namespace) -> None:
    if len(args.gap) != len(args.src):
        raise SystemExit(f"--gap ({len(args.gap)}) 和 --src ({len(args.src)}) 数量必须相同")

    im = Image.open(args.src_img).convert("RGB")
    w, h = im.size
    cx, cy = w // 2, h // 2

    for gap_spec, src_spec in zip(args.gap, args.src):
        gs, ge = (int(x) for x in gap_spec.split(":"))
        ss, se = (int(x) for x in src_spec.split(":"))
        im = seal_gap(im, cx, cy, gs, ge, ss, se, args.r_inner, args.r_outer, args.feather)

    im.save(args.dst, format="PNG", optimize=True)
    print(f"saved: {args.dst}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("src_img", type=Path, help="输入图")
    ap.add_argument("dst", type=Path, help="输出图")
    ap.add_argument("--gap", action="append", required=True, help='缺口角度，如 "13:75"，可多次')
    ap.add_argument("--src", action="append", required=True, help='对应的源弧角度，如 "200:262"')
    ap.add_argument("--r-inner", type=int, default=985, help="复制环带内半径（覆盖发光区下界）")
    ap.add_argument("--r-outer", type=int, default=1035, help="复制环带外半径（覆盖发光区上界）")
    ap.add_argument("--feather", type=int, default=8, help="mask 边缘羽化半径（高斯模糊），0=硬切")
    main(ap.parse_args())
