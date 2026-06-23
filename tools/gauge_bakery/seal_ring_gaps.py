#!/usr/bin/env python3
"""
检测圆形仪表最外圈白环的缺口，画弧线补全。

用法：
  python seal_ring_gaps.py src.png dst.png \
      --radius 1015 --width 4 --color 255,255,255 \
      --gap 13:75 --gap 125:134

或者让脚本自动检测缺口：
  python seal_ring_gaps.py src.png dst.png --auto

角度均用钟表坐标：12 点 = 0°，顺时针为正。
"""
import argparse
import math
from pathlib import Path
from PIL import Image, ImageDraw


def detect_gaps(im: Image.Image, cx: int, cy: int, r: int, threshold: int, min_gap_deg: int) -> list[tuple[int, int]]:
    gray = im.convert("L")
    w, h = gray.size
    gaps = []
    in_gap = False
    start = None
    for deg in range(0, 360):
        rad = math.radians(deg - 90)
        x, y = int(cx + r * math.cos(rad)), int(cy + r * math.sin(rad))
        if not (0 <= x < w and 0 <= y < h):
            continue
        peak = max(gray.getpixel((x + dx, y + dy))
                   for dx in range(-2, 3) for dy in range(-2, 3))
        if peak < threshold:
            if not in_gap:
                in_gap = True
                start = deg
        else:
            if in_gap:
                in_gap = False
                if deg - 1 - start >= min_gap_deg:
                    gaps.append((start, deg - 1))
    if in_gap and 359 - start >= min_gap_deg:
        gaps.append((start, 359))
    return gaps


def main(args: argparse.Namespace) -> None:
    im = Image.open(args.src).convert("RGB")
    w, h = im.size
    cx, cy = w // 2, h // 2
    color = tuple(int(c) for c in args.color.split(","))

    if args.auto:
        gaps = detect_gaps(im, cx, cy, args.radius, args.threshold, args.min_gap_deg)
        print(f"auto-detected gaps: {gaps}")
    else:
        gaps = [tuple(int(x) for x in g.split(":")) for g in args.gap]

    draw = ImageDraw.Draw(im)
    bbox = (cx - args.radius, cy - args.radius, cx + args.radius, cy + args.radius)
    for start, end in gaps:
        # 钟表坐标 → PIL 坐标（PIL: 0° 在 3 点位置）
        pil_start = (start - 90) % 360
        pil_end = (end - 90) % 360
        draw.arc(bbox, start=pil_start, end=pil_end, fill=color, width=args.width)
        print(f"sealed gap: clock {start}°→{end}°  (PIL {pil_start}°→{pil_end}°)")

    im.save(args.dst, format="PNG", optimize=True)
    print(f"saved: {args.dst}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("src", type=Path)
    ap.add_argument("dst", type=Path)
    ap.add_argument("--radius", type=int, default=1015)
    ap.add_argument("--width", type=int, default=4)
    ap.add_argument("--color", type=str, default="255,255,255")
    ap.add_argument("--gap", action="append", default=[], help='缺口角度区间，如 "13:75"，可多次')
    ap.add_argument("--auto", action="store_true", help="自动检测缺口")
    ap.add_argument("--threshold", type=int, default=150)
    ap.add_argument("--min-gap-deg", type=int, default=5)
    main(ap.parse_args())
