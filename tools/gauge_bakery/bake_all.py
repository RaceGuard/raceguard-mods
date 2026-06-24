#!/usr/bin/env python3
"""
批量烘焙所有仪表底图：
  1. 读 gauges.yaml
  2. 每张表渲染 2048×2048 大图（output/raw/）
  3. 缩放到 480×480（圆屏，输出到 src/ui/led/assets/）
  4. 缩放到 420×420（P4 三连表，输出到 src/ui/p4_bar/assets/）

用法：
  python bake_all.py                            # 默认模式: 出 .png 到 LED/P4 assets
  python bake_all.py --only coolant rpm        # 只跑指定的表

主题包模式 (v0.2.0+, 输出到 LittleFS 主题目录):
  python bake_all.py --output-fs <theme_name> \\
      --manifest-author "Designer" --manifest-version "1.0.0"
  # 产物: output/themes/<theme_name>/{manifest.json, gauge_*.png}
  # 然后 cp 到 raceguard-mods/data/themes/<theme_name>/ + pio run -t uploadfs
"""
import argparse
import json
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
THEMES_OUT = SCRIPT_DIR / "output" / "themes"   # FS 主题包输出 (--output-fs 模式)

# 数据源槽位约束 (跟 .a 内 theme_data_sources.cpp 一致)
# 用户主题 manifest.json 的 gauge.name 必须是这 8 个之一,
# .a 内按 name 查 CarData getter/has_value 映射, 不支持自定义数据源 (v0.1.3+ 再加)
VALID_SLOT_NAMES = {"COOLANT", "RPM", "SPEED", "VOLTS", "INTAKE", "AFR", "OIL_TEMP", "BOOST"}

# yaml 里的 name (小写) → manifest 槽位名 (大写) 映射
# (gauges.yaml 用 lowercase 习惯, manifest 必须是上面 8 个之一)
YAML_NAME_TO_SLOT = {
    "coolant": "COOLANT",
    "rpm": "RPM",
    "speed": "SPEED",
    "volts": "VOLTS",
    "intake": "INTAKE",
    "afr": "AFR",
    "oil_temp": "OIL_TEMP",
    "boost": "BOOST",
}

# yaml unit → fmt 推断规则 (manifest 里的 printf 格式串)
def _infer_fmt(unit: str, value_max: float) -> str:
    """从 unit + value_max 推断 printf fmt 串"""
    unit = unit or ""
    if "V" in unit:
        return "%.1fV"          # 电压: 12.3V
    if "°" in unit:
        return f"%.0f{unit}"    # 温度: 95°C
    if unit.lower().startswith("kpa"):
        return "%.0fkPa"
    if value_max > 100:
        return "%.0f"           # 大数值整数 (RPM/SPEED)
    return "%.1f"               # 默认 1 位小数

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


def _build_manifest(theme_name: str, author: str, version: str, gauges_cfg: list, defaults: dict) -> dict:
    """从 gauges.yaml 配置生成 manifest.json 数据结构"""
    angle_start = defaults.get("angle_start", -180.0)
    angle_end = defaults.get("angle_end", 45.0)

    manifest_gauges = []
    for g in gauges_cfg:
        yaml_name = g["name"]
        slot = YAML_NAME_TO_SLOT.get(yaml_name)
        if slot is None:
            print(f"  ⚠️  跳过 '{yaml_name}' (不在 8 个标准槽位内, v0.2.0 主题包只支持 {sorted(VALID_SLOT_NAMES)})")
            continue
        v_min = float(g["value_min"])
        v_max = float(g["value_max"])
        unit = g.get("unit", "")
        # OIL_TEMP / BOOST 跟主仓默认一样, 用户主题保持 disabled (用户自己 enabled = true 启用)
        enabled = slot not in {"OIL_TEMP", "BOOST"}
        manifest_gauges.append({
            "name": slot,
            "file": f"gauge_{yaml_name}.png",
            "min": v_min,
            "max": v_max,
            "angle_start_deg": float(g.get("angle_start", angle_start)),
            "angle_end_deg": float(g.get("angle_end", angle_end)),
            "fmt": _infer_fmt(unit, v_max),
            "enabled_default": enabled,
        })
    return {
        "name": theme_name,
        "author": author,
        "version": version,
        "format": "raceguard-theme-v1",
        "screen": "round-led-21",
        "gauges": manifest_gauges,
    }


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", type=Path, default=SCRIPT_DIR / "gauges.yaml")
    ap.add_argument("--only", nargs="*", default=None, help="只烘焙指定名字的表")
    ap.add_argument("--skip-resize", action="store_true", help="跳过缩放（只出 2048 大图）")
    # v0.2.0+ 主题包模式
    ap.add_argument("--output-fs", metavar="THEME_NAME",
                    help="主题包模式: 输出 PNG + manifest.json 到 output/themes/<THEME_NAME>/, 不写 LED/P4 assets")
    ap.add_argument("--manifest-author", default="Anonymous",
                    help="主题作者 (写入 manifest.json, 默认 Anonymous)")
    ap.add_argument("--manifest-version", default="1.0.0",
                    help="主题版本 (写入 manifest.json, 默认 1.0.0)")
    args = ap.parse_args()

    cfg = yaml.safe_load(args.config.read_text())
    defaults = cfg.get("defaults", {})
    gauges = cfg["gauges"]
    if args.only:
        gauges = [g for g in gauges if g["name"] in args.only]
        if not gauges:
            raise SystemExit(f"--only {args.only} 没匹配到任何表")

    RAW_OUT.mkdir(parents=True, exist_ok=True)

    # 决定输出模式: 主题包 (FS) vs 默认 (LED/P4 assets)
    theme_mode = args.output_fs is not None
    if theme_mode:
        theme_dir = THEMES_OUT / args.output_fs
        theme_dir.mkdir(parents=True, exist_ok=True)
        print(f"===> 主题包模式: 输出到 {theme_dir.relative_to(PROJECT_ROOT)}")
    else:
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

        if theme_mode:
            # FS 主题: 只出 480 PNG, 文件名跟 manifest.gauges[].file 对齐
            out_path = theme_dir / f"gauge_{name}.png"
            resize_to(im, 480).save(out_path, format="PNG", optimize=True)
            print(f"  → {out_path.relative_to(PROJECT_ROOT)}")
        else:
            # 默认模式: 出 LED 480 + P4 420
            led_path = LED_ASSETS / f"gauge_{name}_480.png"
            resize_to(im, 480).save(led_path, format="PNG", optimize=True)
            print(f"  → {led_path.relative_to(PROJECT_ROOT)}")
            p4_path = P4_ASSETS / f"gauge_{name}_420.png"
            resize_to(im, 420).save(p4_path, format="PNG", optimize=True)
            print(f"  → {p4_path.relative_to(PROJECT_ROOT)}")

    if theme_mode:
        manifest = _build_manifest(args.output_fs, args.manifest_author, args.manifest_version,
                                   gauges, defaults)
        manifest_path = theme_dir / "manifest.json"
        manifest_path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False))
        print(f"\n  → {manifest_path.relative_to(PROJECT_ROOT)}")
        print(f"\n完成. 主题 '{args.output_fs}' 已生成 {len(manifest['gauges'])} 张表 + manifest.json")
        print(f"用法:")
        print(f"  cp -r {theme_dir.relative_to(PROJECT_ROOT)} <raceguard-mods>/data/themes/")
        print(f"  cd <raceguard-mods> && pio run -e round-led-21 -t uploadfs")
    else:
        print(f"\n完成。raw 大图在 {RAW_OUT.relative_to(PROJECT_ROOT)}")


if __name__ == "__main__":
    main()
