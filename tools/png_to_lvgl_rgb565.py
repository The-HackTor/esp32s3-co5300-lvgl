#!/usr/bin/env python3
"""Convert a PNG to an LVGL v9 lv_image_dsc_t C source file (RGB565).

Resizes the source PNG (preserving aspect ratio with letterboxing), flattens any
alpha channel onto a solid background, then emits an LVGL image descriptor +
pixel array suitable for `lv_image_set_src()`.

Usage:
    ./tools/png_to_lvgl_rgb565.py INPUT.png OUTPUT.c \
        --name img_face --size 466 --bg ffffff
"""

import argparse
import os
import sys
from PIL import Image, ImageChops, ImageOps


def rgb_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def parse_hex_color(s: str) -> tuple[int, int, int]:
    s = s.lstrip("#")
    if len(s) != 6:
        raise argparse.ArgumentTypeError(f"--bg must be RRGGBB hex, got {s!r}")
    return int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16)


def flatten_on_bg(im: Image.Image, bg_rgb: tuple[int, int, int]) -> Image.Image:
    if im.mode in ("RGBA", "LA"):
        flat = Image.new("RGB", im.size, bg_rgb)
        flat.paste(im, mask=im.split()[-1])
        return flat
    return im.convert("RGB")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("input", help="Source PNG path")
    ap.add_argument("output", help="Destination .c path")
    ap.add_argument("--name", required=True,
                    help="C identifier for the lv_image_dsc_t")
    ap.add_argument("--size", type=int, default=466,
                    help="Target square size in pixels (default 466)")
    ap.add_argument("--bg", default="ffffff",
                    help="Background fill color RRGGBB (default white)")
    ap.add_argument("--trim", action="store_true",
                    help="Auto-crop whitespace (relative to source background)")
    ap.add_argument("--trim-bg", default=None,
                    help="Source background color for trim (default: same as --bg "
                         "BEFORE invert). Use to trim against a white source even "
                         "when output --bg is black.")
    ap.add_argument("--invert", action="store_true",
                    help="Invert RGB channels of the foreground after flatten")
    ap.add_argument("--pad", type=int, default=0,
                    help="Inner padding in pixels around the fitted image")
    ap.add_argument("--anchor", default=None,
                    help="Source-coord point 'cx,cy' to align with canvas center. "
                         "Image is scaled to the largest size that fits within "
                         "(size - 2*pad) while keeping this point at the center.")
    args = ap.parse_args()

    bg_rgb = parse_hex_color(args.bg)
    trim_bg_rgb = parse_hex_color(args.trim_bg) if args.trim_bg else (255, 255, 255)
    side = args.size

    src = Image.open(args.input)
    src.load()

    fitted = flatten_on_bg(src, trim_bg_rgb)
    src_w, src_h = fitted.size

    crop_off_x, crop_off_y = 0, 0
    if args.trim:
        bg_img = Image.new("RGB", fitted.size, trim_bg_rgb)
        diff = ImageChops.difference(fitted, bg_img)
        bbox = diff.getbbox()
        if bbox:
            crop_off_x, crop_off_y = bbox[0], bbox[1]
            fitted = fitted.crop(bbox)

    if args.invert:
        fitted = ImageOps.invert(fitted)

    canvas = Image.new("RGB", (side, side), bg_rgb)

    if args.anchor:
        try:
            ax_str, ay_str = args.anchor.split(",")
            ax_src, ay_src = int(ax_str), int(ay_str)
        except ValueError:
            raise SystemExit("--anchor must be 'cx,cy' integer source coords")
        ax_local = ax_src - crop_off_x
        ay_local = ay_src - crop_off_y
        cw, ch = fitted.size
        inset = args.pad
        center = side / 2.0
        candidates = [
            (center - inset) / ax_local         if ax_local > 0   else float("inf"),
            (center - inset) / (cw - ax_local)  if cw - ax_local > 0 else float("inf"),
            (center - inset) / ay_local         if ay_local > 0   else float("inf"),
            (center - inset) / (ch - ay_local)  if ch - ay_local > 0 else float("inf"),
        ]
        scale = min(candidates)
        new_w = max(1, int(round(cw * scale)))
        new_h = max(1, int(round(ch * scale)))
        fitted = fitted.resize((new_w, new_h), Image.LANCZOS)
        scaled_ax = int(round(ax_local * scale))
        scaled_ay = int(round(ay_local * scale))
        ox = int(round(center)) - scaled_ax
        oy = int(round(center)) - scaled_ay
        canvas.paste(fitted, (ox, oy))
    else:
        inner = max(1, side - 2 * args.pad)
        fitted.thumbnail((inner, inner), Image.LANCZOS)
        ox = (side - fitted.width) // 2
        oy = (side - fitted.height) // 2
        canvas.paste(fitted, (ox, oy))

    pixels = canvas.tobytes()
    n = side * side
    out = bytearray(n * 2)
    for i in range(n):
        r = pixels[3 * i + 0]
        g = pixels[3 * i + 1]
        b = pixels[3 * i + 2]
        px = rgb_to_rgb565(r, g, b)
        out[2 * i + 0] = px & 0xFF
        out[2 * i + 1] = (px >> 8) & 0xFF

    stride = side * 2
    data_size = n * 2
    map_name = f"{args.name}_map"

    lines = []
    lines.append(f"/* Auto-generated by {os.path.basename(__file__)} — do not edit. */")
    lines.append(f"/* Source: {os.path.basename(args.input)} ({side}x{side}, RGB565) */")
    lines.append("")
    lines.append("#include \"lvgl.h\"")
    lines.append("")
    lines.append(f"static const uint8_t {map_name}[] = {{")
    row = []
    for i, byte in enumerate(out):
        row.append(f"0x{byte:02X}")
        if (i + 1) % 16 == 0:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    lines.append("};")
    lines.append("")
    lines.append(f"const lv_image_dsc_t {args.name} = {{")
    lines.append("    .header = {")
    lines.append("        .magic  = LV_IMAGE_HEADER_MAGIC,")
    lines.append("        .cf     = LV_COLOR_FORMAT_RGB565,")
    lines.append("        .flags  = 0,")
    lines.append(f"        .w      = {side},")
    lines.append(f"        .h      = {side},")
    lines.append(f"        .stride = {stride},")
    lines.append("    },")
    lines.append(f"    .data_size = {data_size},")
    lines.append(f"    .data      = {map_name},")
    lines.append("};")
    lines.append("")

    with open(args.output, "w") as f:
        f.write("\n".join(lines))

    print(f"Wrote {args.output} ({data_size} bytes pixel data, {side}x{side})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
