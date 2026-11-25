#!/usr/bin/env python3
"""Font of the game: tools/font/font.png -> data/text_bmp.bin

font.png: 256x64 pixels, 32x8 cells of 8x8 pixels, black background, white
letters. Cell number = character code (Windows-1252 layout, see
make_translations.py for the cells of the Russian letters).

    python3 tools/make_font.py          # font.png -> data/text_bmp.bin
    python3 tools/make_font.py --guide  # also tools/font/font_guide.png:
                                        # enlarged font with the Russian
                                        # letters of each cell
"""
import os
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PNG = os.path.join(ROOT, "tools", "font", "font.png")
BMP = os.path.join(ROOT, "data", "text_bmp.bin")

sys.path.insert(0, os.path.join(ROOT, "tools"))


def cyrillic_cells():
    # Same table as make_translations.py (without running it)
    namespace = {}
    source = open(os.path.join(ROOT, "tools", "make_translations.py"), encoding="utf-8").read()
    start = source.index("CYRILLIC_UPPER =")
    end = source.index("# Russian letters with the same look")
    exec(source[start:end], namespace)
    return namespace["CYRILLIC_CELLS"]


def build():
    image = Image.open(PNG).convert("L")
    if image.size != (256, 64):
        sys.exit("font.png must be 256x64 pixels")
    data = bytearray(open(BMP, "rb").read())
    offset = struct.unpack_from("<I", data, 10)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bits = struct.unpack_from("<H", data, 28)[0]
    if (width, abs(height), bits) != (256, 64, 8):
        sys.exit("data/text_bmp.bin: unexpected format")
    px = image.load()
    for y in range(64):
        row = y if height < 0 else 63 - y
        for x in range(256):
            index = offset + row * 256 + x
            if px[x, y] < 128:
                data[index] = 0
            elif data[index] == 0:
                data[index] = 255  # New pixel (the old ones keep their value)
    open(BMP, "wb").write(data)
    print(f"{PNG} -> {BMP}")


def guide():
    scale = 12
    image = Image.open(PNG).convert("RGB").resize((256 * scale, 64 * scale), Image.NEAREST)
    draw = ImageDraw.Draw(image)
    try:
        font = ImageFont.truetype("DejaVuSans.ttf", 14)
    except OSError:
        font = ImageFont.load_default()
    cells = {cell: letter for letter, cell in cyrillic_cells().items()}
    for cell in range(256):
        x0 = (cell & 31) * 8 * scale
        y0 = (cell >> 5) * 8 * scale
        color = (200, 40, 40) if cell in cells else (60, 60, 60)
        draw.rectangle([x0, y0, x0 + 8 * scale - 1, y0 + 8 * scale - 1], outline=color)
        if cell in cells:
            draw.text((x0 + 8 * scale - 16, y0 + 2), cells[cell], fill=(255, 80, 80), font=font)
        draw.text((x0 + 2, y0 + 8 * scale - 16), str(cell), fill=(90, 140, 255), font=font)
    out = os.path.join(ROOT, "tools", "font", "font_guide.png")
    image.save(out)
    print(out)


build()
if "--guide" in sys.argv:
    guide()
