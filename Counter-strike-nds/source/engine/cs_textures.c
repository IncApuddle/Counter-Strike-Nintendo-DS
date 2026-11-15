// SPDX-License-Identifier: MIT
//
// Copyright (c) 2008-2022 Antonio Niño Díaz
// Copyright (c) 2026, Counter-Strike DS contributors
//
// BMP to NE_PAL256 texture loader with texture wrapping. Based on the BMP
// loader of Nitro Engine (NEFormats.c).

#include <stdint.h>
#include <stdlib.h>

#include "cs_compat.h"

#define CS_MAX_TEXTURE_SIZES 64

static struct {
    const NE_Material *mat;
    int w, h;
} cs_texture_sizes[CS_MAX_TEXTURE_SIZES];

static void cs_texture_size_save(const NE_Material *mat, int w, int h)
{
    int free_slot = -1;
    for (int i = 0; i < CS_MAX_TEXTURE_SIZES; i++)
    {
        if (cs_texture_sizes[i].mat == mat)
        {
            free_slot = i;
            break;
        }
        if (free_slot == -1 && cs_texture_sizes[i].mat == NULL)
            free_slot = i;
    }

    if (free_slot == -1)
    {
        // Table full. Forget the oldest entries: their materials have probably
        // been deleted already.
        for (int i = 0; i < CS_MAX_TEXTURE_SIZES; i++)
            cs_texture_sizes[i].mat = NULL;
        free_slot = 0;
    }

    cs_texture_sizes[free_slot].mat = mat;
    cs_texture_sizes[free_slot].w = w;
    cs_texture_sizes[free_slot].h = h;
}

int CS_MaterialTexLoadBMPtoRGB256(NE_Material *tex, NE_Palette *pal,
                                  const void *pointer, bool transpcolor)
{
    NE_AssertPointer(tex, "NULL material pointer");
    NE_AssertPointer(pal, "NULL palette pointer");
    NE_AssertPointer(pointer, "NULL data pointer");

    if (tex == NULL || pal == NULL || pointer == NULL)
        return 0;

    // The data must be in main RAM. A garbage pointer would crash the game
    // with BlocksDS (the old modified engine just failed to load the texture).
    uintptr_t addr = (uintptr_t)pointer;
    if (addr < 0x02000000 || addr >= 0x03000000)
    {
        NE_DebugPrint("Invalid BMP pointer");
        return 0;
    }

    const NE_BMPHeader *header = pointer;
    const NE_BMPInfoHeader *infoheader =
        (const void *)((const u8 *)header + sizeof(NE_BMPHeader));

    if (header->type != 0x4D42)
    {
        NE_DebugPrint("Not a BMP file");
        return 0;
    }

    int sizex = (int32_t)infoheader->width;
    int sizey = (int32_t)infoheader->height;

    // Negative height means that the image is stored from top to bottom
    bool top_down = false;
    if (sizey < 0)
    {
        sizey = -sizey;
        top_down = true;
    }

    if (sizex <= 0 || sizey <= 0 || sizex > 1024 || sizey > 1024)
    {
        NE_DebugPrint("Invalid BMP size (%d, %d)", sizex, sizey);
        return 0;
    }

    if (infoheader->compression != 0)
    {
        NE_DebugPrint("Compressed BMP not supported");
        return 0;
    }

    int bits = infoheader->bits;
    if (bits != 8 && bits != 4)
    {
        NE_DebugPrint("Unsupported depth for NE_PAL256 conversion (%d)", bits);
        return 0;
    }

    // The palette is right after the info header (its size is in the header)
    const u8 *palettedata = (const u8 *)infoheader + infoheader->size;
    const u8 *imagedata = (const u8 *)header + header->offset;

    int maxcolors = (bits == 8) ? 256 : 16;
    int numcolors = infoheader->ncolors;
    if (numcolors <= 0 || numcolors > maxcolors)
        numcolors = maxcolors;

    u16 *palettebuffer = calloc(256, sizeof(u16));
    if (palettebuffer == NULL)
    {
        NE_DebugPrint("Not enough memory");
        return 0;
    }

    for (int i = 0; i < numcolors; i++)
    {
        u8 b = palettedata[(i << 2) + 0];
        u8 g = palettedata[(i << 2) + 1];
        u8 r = palettedata[(i << 2) + 2];
        palettebuffer[i] = RGB15(r >> 3, g >> 3, b >> 3);
    }

    // Since Nitro Engine 0.15 the width of a texture must be a power of 2 (the
    // old modified engine resized textures automatically). The texture is
    // padded with color 0 up to the next power of 2, and the real size is saved
    // so that only the real part of the texture is drawn (see the CS_2D and
    // CS_Sprite functions below).
    int texwidth = 8;
    while (texwidth < sizex)
        texwidth <<= 1;

    u8 *buffer = calloc(texwidth * sizey, 1);
    if (buffer == NULL)
    {
        NE_DebugPrint("Not enough memory");
        free(palettebuffer);
        return 0;
    }

    // Rows are padded to a multiple of 4 bytes
    int stride = ((sizex * bits + 31) / 32) * 4;

    for (int y = 0; y < sizey; y++)
    {
        int srcy = top_down ? y : (sizey - y - 1);
        const u8 *src = imagedata + srcy * stride;
        u8 *dst = buffer + y * texwidth;

        if (bits == 8)
        {
            for (int x = 0; x < sizex; x++)
                dst[x] = src[x];
        }
        else
        {
            for (int x = 0; x < sizex; x++)
            {
                u8 value = src[x >> 1];
                dst[x] = (x & 1) ? (value & 0x0F) : (value >> 4);
            }
        }
    }

    NE_TextureFlags flags = NE_TEXGEN_TEXCOORD | NE_TEXTURE_WRAP_S
                          | NE_TEXTURE_WRAP_T;
    if (transpcolor)
        flags |= NE_TEXTURE_COLOR0_TRANSPARENT;

    int ret = NE_MaterialTexLoad(tex, NE_PAL256, texwidth, sizey, flags, buffer);
    free(buffer);

    if (ret == 0)
    {
        NE_DebugPrint("Error while loading texture");
        free(palettebuffer);
        return 0;
    }

    // Load the full palette (unused entries are black), like Nitro Engine 0.7.0
    // did, in case some pixel uses an index past the colors stored in the BMP.
    ret = NE_PaletteLoad(pal, palettebuffer, maxcolors, NE_PAL256);
    free(palettebuffer);

    if (ret == 0)
    {
        NE_DebugPrint("Error while loading palette");
        return 0;
    }

    NE_MaterialSetPalette(tex, pal);
    cs_texture_size_save(tex, sizex, sizey);
    return 1;
}

// Real sizes of textures
// ----------------------

void CS_TextureGetRealSize(const NE_Material *mat, int *w, int *h)
{
    for (int i = 0; i < CS_MAX_TEXTURE_SIZES; i++)
    {
        if (cs_texture_sizes[i].mat == mat)
        {
            *w = cs_texture_sizes[i].w;
            *h = cs_texture_sizes[i].h;
            return;
        }
    }

    *w = NE_TextureGetSizeX(mat);
    *h = NE_TextureGetSizeY(mat);
}

void CS_2DDrawTexturedQuad(s16 x1, s16 y1, s16 x2, s16 y2, s16 z,
                           const NE_Material *mat)
{
    int w, h;
    CS_TextureGetRealSize(mat, &w, &h);
    NE_2DDrawTexturedQuadColorCanvas(x1, y1, x2, y2, z, 0, 0, w, h, mat,
                                     mat->color);
}

void CS_2DDrawTexturedQuadColor(s16 x1, s16 y1, s16 x2, s16 y2, s16 z,
                                const NE_Material *mat, u32 color)
{
    int w, h;
    CS_TextureGetRealSize(mat, &w, &h);
    NE_2DDrawTexturedQuadColorCanvas(x1, y1, x2, y2, z, 0, 0, w, h, mat,
                                     color);
}

void CS_SpriteSetMaterial(NE_Sprite *sprite, NE_Material *mat)
{
    int w, h;
    CS_TextureGetRealSize(mat, &w, &h);
    NE_SpriteSetMaterial(sprite, mat);
    NE_SpriteSetMaterialCanvas(sprite, 0, 0, w, h);
    NE_SpriteSetSize(sprite, w, h);
}
