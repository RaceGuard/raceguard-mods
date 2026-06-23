#!/usr/bin/env python3
"""
把圆形仪表底图圆环外的区域填成纯黑。
默认圆心 = 图像中心，半径可通过 --radius 调整（默认 1020px @ 2048 图）。
"""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw


def mask_outside_circle(src: Path, dst: Path, radius: int | None, cx: int | None, cy: int | None) -> None:
    im = Image.open(src).convert("RGB")
    w, h = im.size
    if cx is None:
        cx = w // 2
    if cy is None:
        cy = h // 2
    if radius is None:
        radius = min(cx, cy, w - cx, h - cy) - 4

    mask = Image.new("L", (w, h), 0)
    ImageDraw.Draw(mask).ellipse((cx - radius, cy - radius, cx + radius, cy + radius), fill=255)

    black = Image.new("RGB", (w, h), (0, 0, 0))
    out = Image.composite(im, black, mask)
    out.save(dst, format="PNG", optimize=True)
    print(f"saved: {dst}  size={w}x{h}  center=({cx},{cy})  radius={radius}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("src", type=Path)
    ap.add_argument("dst", type=Path)
    ap.add_argument("--radius", type=int, default=None, help="圆形半径（像素），默认贴边")
    ap.add_argument("--cx", type=int, default=None, help="圆心 X，默认图像中心")
    ap.add_argument("--cy", type=int, default=None, help="圆心 Y，默认图像中心")
    args = ap.parse_args()
    mask_outside_circle(args.src, args.dst, args.radius, args.cx, args.cy)
