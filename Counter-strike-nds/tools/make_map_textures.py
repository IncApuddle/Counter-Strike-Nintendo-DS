#!/usr/bin/env python3
"""Convert the map texture atlases (8 bpp BMP, 256x256) into raw textures.

The game loads data/texMap_*.bin directly into VRAM, without a conversion
buffer in RAM (the conversion failed when there wasn't enough free RAM, like
online after another map: white map).

Format of the .bin files:
    256 u16 palette entries (RGB15, little endian) = 512 bytes
    256 * 256 bytes of pixels (palette indices, top row first)

To edit a texture: edit tools/map_textures/<name>.bmp (8 bpp, 256x256, at most
256 colors, color 0 is transparent) and run:
    python3 tools/make_map_textures.py
"""
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NAMES = ["texMap_Dust2", "texMap_General_Map"]


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
    if (width, height) != (256, 256) or bits != 8 or compression != 0:
        sys.exit(f"{bmp_path}: must be an uncompressed 8 bpp 256x256 BMP "
                 f"(found {width}x{height}, {bits} bpp)")
    if ncolors <= 0 or ncolors > 256:
        ncolors = 256

    pal_start = 14 + info_size
    palette = bytearray()
    for i in range(256):
        if i < ncolors:
            b, g, r = data[pal_start + i * 4: pal_start + i * 4 + 3]
            value = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)
        else:
            value = 0
        palette += struct.pack("<H", value)

    stride = (width * bits + 31) // 32 * 4
    pixels = bytearray()
    for y in range(height):
        src_y = y if top_down else height - 1 - y
        row = data[offset + src_y * stride: offset + src_y * stride + width]
        pixels += row

    with open(bin_path, "wb") as f:
        f.write(palette)
        f.write(pixels)
    print(f"{bmp_path} -> {bin_path}")


for name in NAMES:
    convert(os.path.join(ROOT, "tools", "map_textures", name + ".bmp"),
            os.path.join(ROOT, "data", name + ".bin"))
