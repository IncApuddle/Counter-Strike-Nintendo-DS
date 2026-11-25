#!/usr/bin/env python3
"""Convert the minimap images (8 bpp BMP, at most 16 colors) into raw 16 colors
textures.

The game copies data/MapUI_*.bin directly into VRAM, without a conversion
buffer in RAM (online the conversion could fail: white minimap on Mirage).

Format of the .bin files (little endian):
    u16 width, u16 height, u16 texture width (power of 2), u16 0
    16 u16 palette entries (RGB15)
    texture width / 2 * height bytes: 2 pixels per byte, left pixel in the low
    4 bits, top row first

To edit a minimap: edit tools/map_ui/<name>.bmp (8 bpp, indices 0-15 only) and
run:
    python3 tools/make_map_ui.py
"""
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = ["MapUI_Dust2", "MapUI_General", "MapUI_Mirage"]


def convert(bmp_path, bin_path):
    data = open(bmp_path, "rb").read()
    if data[0:2] != b"BM":
        sys.exit(f"{bmp_path}: not a BMP file")
    offset = struct.unpack_from("<I", data, 10)[0]
    info_size = struct.unpack_from("<I", data, 14)[0]
    width, height = struct.unpack_from("<ii", data, 18)
    bits = struct.unpack_from("<H", data, 28)[0]
    compression = struct.unpack_from("<I", data, 30)[0]
    ncolors = struct.unpack_from("<I", data, 46)[0]
    top_down = height < 0
    height = abs(height)
    if bits != 8 or compression != 0:
        sys.exit(f"{bmp_path}: must be an uncompressed 8 bpp BMP")
    if ncolors <= 0 or ncolors > 256:
        ncolors = 256

    texwidth = 8
    while texwidth < width:
        texwidth <<= 1

    pal_start = 14 + info_size
    palette = bytearray()
    for i in range(16):
        if i < ncolors:
            b, g, r = data[pal_start + i * 4: pal_start + i * 4 + 3]
            value = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)
        else:
            value = 0
        palette += struct.pack("<H", value)

    stride = (width * bits + 31) // 32 * 4
    pixels = bytearray(texwidth // 2 * height)
    for y in range(height):
        src_y = y if top_down else height - 1 - y
        row = data[offset + src_y * stride: offset + src_y * stride + width]
        for x, index in enumerate(row):
            if index > 15:
                sys.exit(f"{bmp_path}: color index {index} at ({x}, {y}), "
                         "only 16 colors (indices 0-15) are allowed")
            if x & 1:
                pixels[y * (texwidth // 2) + (x >> 1)] |= index << 4
            else:
                pixels[y * (texwidth // 2) + (x >> 1)] |= index

    with open(bin_path, "wb") as f:
        f.write(struct.pack("<HHHH", width, height, texwidth, 0))
        f.write(palette)
        f.write(pixels)
    print(f"{bmp_path} -> {bin_path}")


for name in NAMES:
    convert(os.path.join(ROOT, "tools", "map_ui", name + ".bmp"),
            os.path.join(ROOT, "data", name + ".bin"))
