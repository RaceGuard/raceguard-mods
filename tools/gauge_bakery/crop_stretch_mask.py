#!/usr/bin/env python3
"""
处理 AI 生成的非正圆底图：
  1. 裁掉左右两侧的装饰带（crop_left / crop_right 像素）
  2. 横向拉伸回原宽度，让"高瘦"的仪表变饱满正圆
  3. 圆外像素填纯黑（mask）

用法：
  python crop_stretch_mask.py background.png background_round.png \
      --crop-left 120 --crop-right 120
"""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw


def process(args: argparse.Namespace) -> None:
    im = Image.open(args.src).convert("RGB")
    w, h = im.size

    # 1. 裁
    cropped = im.crop((args.crop_left, 0, w - args.crop_right, h))
    cw, ch = cropped.size
    print(f"crop: {w}x{h} → {cw}x{ch}")

    # 2. 横向拉伸回原宽（高度不变）
    stretched = cropped.resize((w, h), Image.LANCZOS)

    # 3. mask 圆外为黑
    cx, cy = w // 2, h // 2
    radius = args.radius if args.radius is not None else min(cx, cy) - 4
    mask = Image.new("L", (w, h), 0)
    ImageDraw.Draw(mask).ellipse((cx - radius, cy - radius, cx + radius, cy + radius), fill=255)
    black = Image.new("RGB", (w, h), (0, 0, 0))
    out = Image.composite(stretched, black, mask)

    out.save(args.dst, format="PNG", optimize=True)
    print(f"saved: {args.dst}  center=({cx},{cy})  radius={radius}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("src", type=Path)
    ap.add_argument("dst", type=Path)
    ap.add_argument("--crop-left", type=int, default=120)
    ap.add_argument("--crop-right", type=int, default=120)
    ap.add_argument("--radius", type=int, default=None)
    process(ap.parse_args())
