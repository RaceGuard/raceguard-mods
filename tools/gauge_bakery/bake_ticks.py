#!/usr/bin/env python3
"""
在圆形仪表底图上烘焙刻度线 + 数字标签（带光泽渐变）。

约定：角度采用"钟表坐标"——12 点为 0°，顺时针为正。
  - -90°  = 9 点位置（左）
  - 0°    = 12 点（顶）
  - 90°   = 3 点（右）
  - 135°  = 4-5 点之间（右下）

用法示例（水温 60-140 ℃，跨度 -90° → +135°）：
  python bake_ticks.py background_masked.png gauge_coolant.png \
      --value-min 60 --value-max 140 --major-step 20 --minor-div 5 \
      --angle-start -90 --angle-end 135 \
      --labels 60 80 100 120 140
"""
import argparse
import math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter


FONT_PATH_DEFAULT = "/System/Library/Fonts/Supplemental/DIN Alternate Bold.ttf"


def polar(cx: float, cy: float, r: float, deg_clock: float) -> tuple[float, float]:
    """钟表坐标 → 屏幕像素。0° 在 12 点，顺时针为正。"""
    rad = math.radians(deg_clock - 90)  # 转成数学坐标（0° 在 3 点，逆时针为正）
    return cx + r * math.cos(rad), cy + r * math.sin(rad)


def make_text_glossy(text: str, font: ImageFont.FreeTypeFont,
                     top_color=(255, 255, 255), bot_color=(168, 192, 216),
                     mid_color=None, mid_pos: float = 0.5,
                     stroke_width: int = 0, stroke_color=(255, 255, 255)) -> Image.Image:
    """渲染带垂直渐变 + 可选描边的文字 RGBA 图层。

    mid_color 不为 None 时使用三段渐变（top→mid→bot），mid_pos 控制中间色的位置（0-1）。
    """
    bbox = font.getbbox(text)
    pad = 8 + stroke_width * 2
    w = bbox[2] - bbox[0] + pad * 2
    h = bbox[3] - bbox[1] + pad * 2
    ox, oy = -bbox[0] + pad, -bbox[1] + pad

    # 字形 alpha mask（含描边）
    mask = Image.new("L", (w, h), 0)
    md = ImageDraw.Draw(mask)
    if stroke_width > 0:
        md.text((ox, oy), text, fill=255, font=font,
                stroke_width=stroke_width, stroke_fill=255)
    else:
        md.text((ox, oy), text, fill=255, font=font)

    # 渐变背景（覆盖整图，后用 mask 切出字形）
    grad = Image.new("RGB", (w, h))
    px = grad.load()
    for y in range(h):
        t = y / max(h - 1, 1)
        if mid_color is not None:
            if t < mid_pos:
                k = t / mid_pos
                c1, c2 = top_color, mid_color
            else:
                k = (t - mid_pos) / max(1 - mid_pos, 0.001)
                c1, c2 = mid_color, bot_color
        else:
            k = t
            c1, c2 = top_color, bot_color
        r = int(c1[0] * (1 - k) + c2[0] * k)
        g = int(c1[1] * (1 - k) + c2[1] * k)
        b = int(c1[2] * (1 - k) + c2[2] * k)
        for x in range(w):
            px[x, y] = (r, g, b)

    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    if stroke_width > 0:
        # 先用纯描边色填充字形（含描边），再用渐变覆盖字形内部（不含描边）
        stroke_layer = Image.new("RGB", (w, h), stroke_color)
        out.paste(stroke_layer, (0, 0), mask)
        # 字形内部 mask（无描边）
        inner_mask = Image.new("L", (w, h), 0)
        ImageDraw.Draw(inner_mask).text((ox, oy), text, fill=255, font=font)
        out.paste(grad, (0, 0), inner_mask)
    else:
        out.paste(grad, (0, 0), mask)
    return out


def apply_drop_shadow(glyph: Image.Image, offset: tuple[int, int],
                      blur: int, color: tuple[int, int, int]) -> Image.Image:
    """在文字下方加偏移的羽化阴影。"""
    w, h = glyph.size
    pad = max(abs(offset[0]), abs(offset[1])) + blur * 2
    canvas = Image.new("RGBA", (w + pad * 2, h + pad * 2), (0, 0, 0, 0))
    alpha = glyph.split()[-1]
    shadow_rgb = Image.new("RGB", glyph.size, color)
    shadow = Image.new("RGBA", glyph.size, (0, 0, 0, 0))
    shadow.paste(shadow_rgb, (0, 0), alpha)
    shadow = shadow.filter(ImageFilter.GaussianBlur(blur))
    canvas.alpha_composite(shadow, (pad + offset[0], pad + offset[1]))
    canvas.alpha_composite(glyph, (pad, pad))
    return canvas


def apply_emboss(glyph: Image.Image, text: str, font: ImageFont.FreeTypeFont,
                 offset: int, highlight: tuple[int, int, int],
                 shadow: tuple[int, int, int]) -> Image.Image:
    """字顶部白线 + 底部深线，模拟凸起浮雕。"""
    w, h = glyph.size
    pad = offset + 2
    canvas = Image.new("RGBA", (w + pad * 2, h + pad * 2), (0, 0, 0, 0))
    bbox = font.getbbox(text)
    mask = Image.new("L", (w, h), 0)
    ox, oy = -bbox[0] + (w - (bbox[2] - bbox[0])) // 2, -bbox[1] + (h - (bbox[3] - bbox[1])) // 2
    ImageDraw.Draw(mask).text((ox, oy), text, fill=255, font=font)

    hl_layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    Image.new("RGB", (w, h), highlight)  # ensure import
    hl_solid = Image.new("RGB", (w, h), highlight)
    hl_layer.paste(hl_solid, (0, 0), mask)

    sh_layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    sh_solid = Image.new("RGB", (w, h), shadow)
    sh_layer.paste(sh_solid, (0, 0), mask)

    # 顺序：先底部阴影、再顶部高光、最后主字（覆盖中间）
    canvas.alpha_composite(sh_layer, (pad, pad + offset))
    canvas.alpha_composite(hl_layer, (pad, pad - offset))
    canvas.alpha_composite(glyph, (pad, pad))
    return canvas


def apply_outer_glow(glyph: Image.Image, color: tuple[int, int, int],
                     radius: int, intensity: float) -> Image.Image:
    """字周围添加发光晕。"""
    w, h = glyph.size
    pad = radius * 2
    canvas = Image.new("RGBA", (w + pad * 2, h + pad * 2), (0, 0, 0, 0))
    alpha = glyph.split()[-1]
    glow_alpha = alpha.filter(ImageFilter.GaussianBlur(radius))
    glow_alpha = glow_alpha.point(lambda v: min(int(v * intensity), 255))
    glow_rgb = Image.new("RGB", glyph.size, color)
    glow = Image.new("RGBA", glyph.size, (0, 0, 0, 0))
    glow.paste(glow_rgb, (0, 0), glow_alpha)
    canvas.alpha_composite(glow, (pad, pad))
    canvas.alpha_composite(glyph, (pad, pad))
    return canvas


def bake(args: argparse.Namespace) -> None:
    im = Image.open(args.src).convert("RGBA")
    w, h = im.size
    cx = args.cx if args.cx is not None else w // 2
    cy = args.cy if args.cy is not None else h // 2

    draw = ImageDraw.Draw(im)

    span = args.angle_end - args.angle_start
    value_range = args.value_max - args.value_min

    def value_to_pil_angle(v: float) -> float:
        clock = args.angle_start + (v - args.value_min) / value_range * span
        return (clock - 90) % 360

    # 告警色带（先画，让刻度线压在上面）——外缘饱和、内缘透明的径向渐变
    def draw_zone_band(v_start: float, v_end: float, rgb: tuple[int, int, int]) -> None:
        layer = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        layer_draw = ImageDraw.Draw(layer)
        pil_start = value_to_pil_angle(v_start)
        pil_end = value_to_pil_angle(v_end)
        span_r = args.zone_r_outer - args.zone_r_inner
        for r in range(args.zone_r_outer, args.zone_r_inner - 1, -1):
            t = (args.zone_r_outer - r) / max(span_r, 1)  # 0=外缘, 1=内缘
            alpha = int(255 * (1 - t) ** 1.2)  # 略微非线性，使内侧消失更快
            layer_draw.arc((cx - r, cy - r, cx + r, cy + r),
                           start=pil_start, end=pil_end,
                           fill=(*rgb, alpha), width=1)
        im.alpha_composite(layer)

    if args.warn_from is not None:
        warn_end = args.danger_from if args.danger_from is not None else args.value_max
        draw_zone_band(args.warn_from, warn_end, (255, 200, 30))
    if args.danger_from is not None:
        draw_zone_band(args.danger_from, args.value_max, (230, 40, 40))

    # 刻度线
    minor_step = args.major_step / args.minor_div
    total_minor = int(round(value_range / minor_step))

    def tick_color(v: float, is_major: bool) -> tuple[int, int, int, int]:
        return (255, 255, 255, 255) if is_major else (170, 180, 190, 220)

    for i in range(total_minor + 1):
        v = args.value_min + i * minor_step
        is_major = abs((v - args.value_min) % args.major_step) < 1e-6
        ang = args.angle_start + (v - args.value_min) / value_range * span
        if is_major:
            r_out = args.tick_radius
            r_in = args.tick_radius - args.tick_major_len
            width = args.tick_major_width
        else:
            r_out = args.tick_radius
            r_in = args.tick_radius - args.tick_minor_len
            width = args.tick_minor_width
        p1 = polar(cx, cy, r_out, ang)
        p2 = polar(cx, cy, r_in, ang)
        draw.line([p1, p2], fill=tick_color(v, is_major), width=width)

    # 数字标签
    font = ImageFont.truetype(args.font, args.font_size)

    def label_colors(v: float) -> tuple[tuple[int, int, int], tuple[int, int, int]]:
        if args.danger_from is not None and v >= args.danger_from:
            return (255, 140, 140), (210, 30, 30)
        if args.warn_from is not None and v >= args.warn_from:
            return (255, 215, 80), (210, 150, 15)
        return (255, 255, 255), (168, 192, 216)

    for label in args.labels:
        # "value:text" → 显示文本和位置值分开（如 "1000:1"）；纯数字则两者一致
        if ":" in str(label):
            v_str, text = str(label).split(":", 1)
            v = float(v_str)
        else:
            v = float(label)
            text = str(label)
        ang = args.angle_start + (v - args.value_min) / value_range * span
        lx, ly = polar(cx, cy, args.label_radius, ang)
        top, bot = label_colors(v)
        glyph = make_text_glossy(text, font, top_color=top, bot_color=bot)
        gw, gh = glyph.size
        im.alpha_composite(glyph, (int(lx - gw / 2), int(ly - gh / 2)))

    def render_glossy_text(text: str, font_path: str, font_size: int,
                           effect: str) -> Image.Image:
        f = ImageFont.truetype(font_path, font_size)
        g = make_text_glossy(text, f, top_color=(255, 255, 255), bot_color=(60, 90, 130))
        if effect == "shadow":
            g = apply_drop_shadow(g, offset=(6, 10), blur=6, color=(0, 0, 20))
        elif effect == "emboss":
            g = apply_emboss(g, text, f, offset=2,
                             highlight=(255, 255, 255), shadow=(0, 10, 30))
        elif effect == "glow":
            g = apply_outer_glow(g, color=(80, 160, 240), radius=18, intensity=1.8)
        return g

    # 标题
    if args.title:
        title_glyph = render_glossy_text(
            args.title, args.title_font or args.font,
            args.title_font_size, args.title_effect)
        tw, th = title_glyph.size
        im.alpha_composite(title_glyph,
                           (int(args.title_x - tw / 2), int(args.title_y - th / 2)))

    # 单位（放下卡片，靠右对齐——anchor='right'：unit-x 为右边界 x）
    if args.unit:
        unit_glyph = render_glossy_text(
            args.unit, args.unit_font or args.font,
            args.unit_font_size, args.unit_effect)
        uw, uh = unit_glyph.size
        if args.unit_anchor == "right":
            ux = args.unit_x - uw
        elif args.unit_anchor == "center":
            ux = int(args.unit_x - uw / 2)
        else:  # left
            ux = args.unit_x
        im.alpha_composite(unit_glyph, (int(ux), int(args.unit_y - uh / 2)))

    im.convert("RGB").save(args.dst, format="PNG", optimize=True)
    print(f"saved: {args.dst}  ticks={total_minor + 1}  labels={len(args.labels)}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(formatter_class=argparse.RawTextHelpFormatter, description=__doc__)
    ap.add_argument("src", type=Path)
    ap.add_argument("dst", type=Path)
    ap.add_argument("--cx", type=int, default=None)
    ap.add_argument("--cy", type=int, default=None)
    ap.add_argument("--value-min", type=float, required=True)
    ap.add_argument("--value-max", type=float, required=True)
    ap.add_argument("--major-step", type=float, required=True, help="主刻度间隔（值单位）")
    ap.add_argument("--minor-div", type=int, default=5, help="每个主刻度内分几格次刻度")
    ap.add_argument("--angle-start", type=float, required=True, help="起始角度（钟表坐标，12 点为 0°）")
    ap.add_argument("--angle-end", type=float, required=True, help="结束角度")
    ap.add_argument("--labels", type=str, nargs="+", required=True, help="要标注的数字列表")
    ap.add_argument("--tick-radius", type=int, default=900, help="刻度线外端半径")
    ap.add_argument("--tick-major-len", type=int, default=42)
    ap.add_argument("--tick-minor-len", type=int, default=20)
    ap.add_argument("--tick-major-width", type=int, default=8)
    ap.add_argument("--tick-minor-width", type=int, default=3)
    ap.add_argument("--label-radius", type=int, default=780, help="数字标签中心半径")
    ap.add_argument("--font", type=str, default=FONT_PATH_DEFAULT)
    ap.add_argument("--font-size", type=int, default=90)
    ap.add_argument("--title", type=str, default="", help="仪表标题文字（留空则不画）")
    ap.add_argument("--title-x", type=int, default=1500)
    ap.add_argument("--title-y", type=int, default=680)
    ap.add_argument("--title-font", type=str, default="", help="标题字体路径，默认同 --font")
    ap.add_argument("--title-font-size", type=int, default=80)
    ap.add_argument("--title-stroke-width", type=int, default=0, help="（已弃用）金属描边粗细")
    ap.add_argument("--title-effect", choices=["none", "shadow", "emboss", "glow"],
                    default="none", help="标题立体效果")
    ap.add_argument("--warn-from", type=float, default=None, help="≥此值的数字用黄色，对应弧段画黄色色带")
    ap.add_argument("--danger-from", type=float, default=None, help="≥此值的数字用红色，对应弧段画红色色带")
    ap.add_argument("--zone-r-inner", type=int, default=1014, help="告警色带内半径")
    ap.add_argument("--zone-r-outer", type=int, default=1024, help="告警色带外半径")
    ap.add_argument("--unit", type=str, default="", help="单位（放下卡片，如 \"°C\"）")
    ap.add_argument("--unit-x", type=int, default=1980, help="单位锚点 x（按 --unit-anchor 解释）")
    ap.add_argument("--unit-y", type=int, default=1380, help="单位中心 y")
    ap.add_argument("--unit-anchor", choices=["left", "center", "right"], default="right",
                    help="unit-x 对应单位的哪个对齐边")
    ap.add_argument("--unit-font", type=str, default="")
    ap.add_argument("--unit-font-size", type=int, default=180)
    ap.add_argument("--unit-effect", choices=["none", "shadow", "emboss", "glow"], default="shadow")
    bake(ap.parse_args())
