#!/usr/bin/env python3
"""
把"不太圆"的圆形仪表底图强制变成正圆：
  1. 先用保守半径 mask 掉左右装饰带（避免找 bbox 时被干扰）
  2. 找白色圆环外圈的 bbox（最严格的上下左右边界）
  3. 裁掉 bbox 外的黑色
  4. resize 成正方形 → 椭圆环被强制拉成正圆
  5. 最终再 mask 一次圆外为黑

用法：
  python snap_to_circle.py background.png background_round_final.png
"""
import argparse
from pathlib import Path
from PIL import Image, ImageDraw


def find_white_bbox(im: Image.Image, threshold: int) -> tuple[int, int, int, int]:
    gray = im.convert("L")
    w, h = gray.size
    px = gray.load()
    left, right, top, bot = w, 0, h, 0
    for y in range(h):
        for x in range(w):
            if px[x, y] > threshold:
                if x < left: left = x
                if x > right: right = x
                if y < top: top = y
                if y > bot: bot = y
    return left, top, right, bot


def main(args: argparse.Namespace) -> None:
    im = Image.open(args.src).convert("RGB")
    w, h = im.size
    cx, cy = w // 2, h // 2

    # 1. 保守 mask 掉左右装饰
    pre_r = args.pre_radius if args.pre_radius is not None else min(cx, cy) - 4
    pre_mask = Image.new("L", (w, h), 0)
    ImageDraw.Draw(pre_mask).ellipse((cx - pre_r, cy - pre_r, cx + pre_r, cy + pre_r), fill=255)
    black = Image.new("RGB", (w, h), (0, 0, 0))
    masked = Image.composite(im, black, pre_mask)

    # 2. 找白像素 bbox
    left, top, right, bot = find_white_bbox(masked, args.threshold)
    bw, bh = right - left + 1, bot - top + 1
    print(f"white-ring bbox: x∈[{left},{right}] y∈[{top},{bot}]  → {bw}x{bh}")

    # 3. crop
    cropped = masked.crop((left, top, right + 1, bot + 1))

    # 4. resize to square
    size = args.target_size if args.target_size else max(bw, bh)
    resized = cropped.resize((size, size), Image.LANCZOS)
    print(f"resize: {bw}x{bh} → {size}x{size}  (stretch ratio: {size/bw:.3f}x / {size/bh:.3f}y)")

    # 5. final mask
    final_r = size // 2 - 2
    final_mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(final_mask).ellipse(
        (size // 2 - final_r, size // 2 - final_r, size // 2 + final_r, size // 2 + final_r),
        fill=255,
    )
    black2 = Image.new("RGB", (size, size), (0, 0, 0))
    final = Image.composite(resized, black2, final_mask)
    final.save(args.dst, format="PNG", optimize=True)
    print(f"saved: {args.dst}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("src", type=Path)
    ap.add_argument("dst", type=Path)
    ap.add_argument("--pre-radius", type=int, default=None, help="预 mask 半径（保守地遮掉左右装饰带）")
    ap.add_argument("--threshold", type=int, default=180, help="白像素亮度阈值")
    ap.add_argument("--target-size", type=int, default=None, help="目标正方形尺寸，默认取 bbox 较长边")
    main(ap.parse_args())
