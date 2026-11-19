#!/usr/bin/env python3
# Atlas of the small gun icons used for the guns dropped on the floor.
#
# The atlas is a 256x128 image: 8 columns x 4 rows of 32x32 cells. Cell N is
# the gun AllGuns[N + 1] (row 1: pistols then M3, XM1014, row 2: M249 to
# Galil, row 3: AK-47 to G3SG1, row 4: unused).
#
# Usage:
#   make_dropped_guns_atlas.py export <data dir> <atlas.png>
#       Make an editable PNG from the shop icons of the game (data/*.bin).
#   make_dropped_guns_atlas.py build <atlas.png> <dropped_guns.bin> [preview.png]
#       Convert the PNG to the format of the game.
#
# The PNG for "build" can be:
#   - an RGBA PNG (any colors): transparent pixels (alpha < 128) stay
#     transparent, the other colors are reduced to 15 colors.
#   - an indexed PNG with 16 colors at most: used as it is (to control the
#     colors exactly). The transparent color of the PNG is used, or index 0
#     if the PNG has no transparent color.
#
# Output format (raw, loaded with NE_MaterialTexLoad(NE_PAL16)):
#   32 bytes: palette (16 colors, RGB15 little endian, color 0 = transparent)
#   16384 bytes: 4 bpp texels (low nibble = left pixel)
import sys
from PIL import Image

GUNS = ("glock18 uspsilencer p228 deagle elite fivesevent m3super90 xm1014 "
        "m249 mac10 tmp mp5 ump45 p90 famas galil ak47 scout m4a1 aug sg552 "
        "sg550 awp g3sg1").split()  # Same order as AllGuns[1..24]
CELL = 32
COLS = 8
ATLAS_W, ATLAS_H = 256, 128


def load_icon(path):
    im = Image.open(path)
    # Palette index 0 is the transparent color
    mask = Image.frombytes("L", im.size, bytes(0 if v == 0 else 255 for v in im.tobytes()))
    bbox = mask.getbbox()
    rgba = im.convert("RGBA")
    rgba.putalpha(mask)
    rgba = rgba.crop(bbox)
    w, h = rgba.size
    s = CELL / max(w, h)
    rgba = rgba.resize((max(1, round(w * s)), max(1, round(h * s))), Image.LANCZOS)
    cell = Image.new("RGBA", (CELL, CELL), (0, 0, 0, 0))
    cell.paste(rgba, ((CELL - rgba.size[0]) // 2, (CELL - rgba.size[1]) // 2), rgba)
    return cell


def export(data_dir, png_path):
    atlas = Image.new("RGBA", (ATLAS_W, ATLAS_H), (0, 0, 0, 0))
    for i, name in enumerate(GUNS):
        cell = load_icon(f"{data_dir}/{name}.bin")
        atlas.paste(cell, ((i % COLS) * CELL, (i // COLS) * CELL))
    # Hard alpha (the game has no semi transparency)
    alpha = atlas.getchannel("A").point(lambda a: 255 if a >= 128 else 0)
    atlas.putalpha(alpha)
    atlas.save(png_path)


def to_indices(png_path):
    im = Image.open(png_path)
    if im.size != (ATLAS_W, ATLAS_H):
        sys.exit(f"The atlas must be {ATLAS_W}x{ATLAS_H} (it is {im.size[0]}x{im.size[1]})")

    # Indexed image with 16 colors at most: used as it is. The transparent
    # color of the PNG (if any) becomes index 0 (swapped with index 0)
    transparency = im.info.get("transparency")
    if im.mode == "P" and max(im.tobytes()) < 16 and (transparency is None or isinstance(transparency, int)):
        pal = im.getpalette()[:16 * 3]
        pal += [0] * (16 * 3 - len(pal))
        colors = [tuple(pal[i * 3:i * 3 + 3]) for i in range(16)]
        idx = list(im.tobytes())
        t = transparency if transparency is not None else 0
        if t != 0:
            if t >= 16:
                sys.exit("The transparent color must be one of the 16 first colors")
            colors[0], colors[t] = colors[t], colors[0]
            idx = [0 if v == t else (t if v == 0 else v) for v in idx]
        return idx, colors

    # Any other image: 15 colors + transparent
    rgba = im.convert("RGBA")
    alpha = list(rgba.getchannel("A").tobytes())
    rgb = rgba.convert("RGB")
    opaque_pixels = [p for p, a in zip(rgb.get_flattened_data(), alpha) if a >= 128]
    if not opaque_pixels:
        sys.exit("The atlas is empty")
    sample = Image.new("RGB", (len(opaque_pixels), 1))
    sample.putdata(opaque_pixels)
    q = sample.quantize(colors=15, method=Image.MEDIANCUT)
    pal = q.getpalette()[:15 * 3]
    pal += [0] * (15 * 3 - len(pal))
    quant = list(rgb.quantize(palette=q, dither=Image.NONE).tobytes())
    idx = [0 if a < 128 else v + 1 for v, a in zip(quant, alpha)]
    colors = [(0, 0, 0)] + [tuple(pal[i * 3:i * 3 + 3]) for i in range(15)]
    return idx, colors


def build(png_path, out_path, preview_path=None):
    idx, colors = to_indices(png_path)

    out = bytearray()
    for r, g, b in colors:
        v = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)
        out += bytes((v & 0xFF, v >> 8))
    for y in range(ATLAS_H):
        for x in range(0, ATLAS_W, 2):
            out.append(idx[y * ATLAS_W + x] | (idx[y * ATLAS_W + x + 1] << 4))
    with open(out_path, "wb") as f:
        f.write(out)

    if preview_path:
        prev = Image.new("RGB", (ATLAS_W, ATLAS_H), (90, 80, 60))
        for i, v in enumerate(idx):
            if v:
                prev.putpixel((i % ATLAS_W, i // ATLAS_W), colors[v])
        prev.resize((ATLAS_W * 3, ATLAS_H * 3), Image.NEAREST).save(preview_path)


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == "export":
        export(sys.argv[2], sys.argv[3])
    elif len(sys.argv) >= 4 and sys.argv[1] == "build":
        build(sys.argv[2], sys.argv[3], sys.argv[4] if len(sys.argv) > 4 else None)
    else:
        sys.exit(__doc__ if __doc__ else "See the comment at the top of the script")


if __name__ == "__main__":
    main()
