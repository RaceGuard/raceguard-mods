#!/usr/bin/env python3
"""
批量烘焙所有仪表底图：
  1. 读 gauges.yaml
  2. 每张表渲染 2048×2048 大图（output/raw/）
  3. 缩放到 480×480（圆屏，输出到 src/ui/led/assets/）
  4. 缩放到 420×420（P4 三连表，输出到 src/ui/p4_bar/assets/）

用法：
  python bake_all.py                    # 全跑
  python bake_all.py --only coolant rpm # 只跑指定
"""
import argparse
import sys
from pathlib import Path
from types import SimpleNamespace
import yaml
from PIL import Image

# 同目录的 bake_ticks 模块
SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))
import bake_ticks  # noqa: E402

PROJECT_ROOT = SCRIPT_DIR.parent.parent
LED_ASSETS = PROJECT_ROOT / "src" / "ui" / "led" / "assets"
P4_ASSETS = PROJECT_ROOT / "src" / "ui" / "p4_bar" / "assets"
RAW_OUT = SCRIPT_DIR / "output" / "raw"

# 这些字段如果 gauge 没指定就用 defaults 里的，全 namespace 都要齐
# bake_ticks.bake 期望的字段（见 bake_ticks.py argparse 定义）
REQUIRED_FIELDS = [
    "src", "dst", "cx", "cy",
    "value_min", "value_max", "major_step", "minor_div",
    "angle_start", "angle_end", "labels",
    "tick_radius", "tick_major_len", "tick_minor_len",
    "tick_major_width", "tick_minor_width",
    "label_radius", "font", "font_size",
    "title", "title_x", "title_y", "title_font", "title_font_size",
    "title_stroke_width", "title_effect",
    "warn_from", "danger_from", "zone_r_inner", "zone_r_outer",
    "unit", "unit_x", "unit_y", "unit_anchor", "unit_font", "unit_font_size", "unit_effect",
]

# 字段缺省（bake_ticks argparse 的默认值副本，省去 None 检查麻烦）
DEFAULTS = {
    "cx": None, "cy": None,
    "title": "", "title_font": "", "title_stroke_width": 0, "title_effect": "none",
    "warn_from": None, "danger_from": None,
    "unit": "", "unit_font": "",
    "unit_anchor": "right", "unit_effect": "none",
}


def to_namespace(gauge_cfg: dict, defaults: dict, dst: Path) -> SimpleNamespace:
    merged = {**DEFAULTS, **defaults, **gauge_cfg, "dst": dst}
    # 把相对路径 src 转绝对
    src = Path(merged["src"])
    if not src.is_absolute():
        src = SCRIPT_DIR / src
    merged["src"] = src
    missing = [f for f in REQUIRED_FIELDS if f not in merged]
    if missing:
        raise SystemExit(f"配置缺字段：{missing}")
    return SimpleNamespace(**merged)


def resize_to(src_img: Image.Image, size: int) -> Image.Image:
    """高质量缩放到 size×size。"""
    return src_img.resize((size, size), Image.LANCZOS)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", type=Path, default=SCRIPT_DIR / "gauges.yaml")
    ap.add_argument("--only", nargs="*", default=None, help="只烘焙指定名字的表")
    ap.add_argument("--skip-resize", action="store_true", help="跳过缩放（只出 2048 大图）")
    args = ap.parse_args()

    cfg = yaml.safe_load(args.config.read_text())
    defaults = cfg.get("defaults", {})
    gauges = cfg["gauges"]
    if args.only:
        gauges = [g for g in gauges if g["name"] in args.only]
        if not gauges:
            raise SystemExit(f"--only {args.only} 没匹配到任何表")

    RAW_OUT.mkdir(parents=True, exist_ok=True)
    LED_ASSETS.mkdir(parents=True, exist_ok=True)
    P4_ASSETS.mkdir(parents=True, exist_ok=True)

    for g in gauges:
        name = g["name"]
        print(f"\n=== {name} ===")
        raw_path = RAW_OUT / f"gauge_{name}.png"
        ns = to_namespace(g, defaults, raw_path)
        bake_ticks.bake(ns)

        if args.skip_resize:
            continue

        im = Image.open(raw_path)
        led_path = LED_ASSETS / f"gauge_{name}_480.png"
        resize_to(im, 480).save(led_path, format="PNG", optimize=True)
        print(f"  → {led_path.relative_to(PROJECT_ROOT)}")

        p4_path = P4_ASSETS / f"gauge_{name}_420.png"
        resize_to(im, 420).save(p4_path, format="PNG", optimize=True)
        print(f"  → {p4_path.relative_to(PROJECT_ROOT)}")

    print(f"\n完成。raw 大图在 {RAW_OUT.relative_to(PROJECT_ROOT)}")


if __name__ == "__main__":
    main()
