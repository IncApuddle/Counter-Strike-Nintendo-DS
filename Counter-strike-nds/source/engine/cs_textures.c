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

int CS_TexturePrepareBMP(CS_PreparedTexture *out, const void *pointer)
{
    NE_AssertPointer(out, "NULL prepared texture pointer");
    NE_AssertPointer(pointer, "NULL data pointer");

    if (out == NULL || pointer == NULL)
        return 0;

    out->buffer = NULL;
    out->palette = NULL;

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

    out->buffer = buffer;
    out->palette = palettebuffer;
    out->texwidth = texwidth;
    out->width = sizex;
    out->height = sizey;
    out->numcolors = maxcolors;
    return 1;
}

void CS_TextureFreePrepared(CS_PreparedTexture *prepared)
{
    if (prepared == NULL)
        return;
    free(prepared->buffer);
    free(prepared->palette);
    prepared->buffer = NULL;
    prepared->palette = NULL;
}

int CS_MaterialTexLoadPrepared(NE_Material *tex, NE_Palette *pal,
                               const CS_PreparedTexture *prepared,
                               bool transpcolor)
{
    NE_AssertPointer(tex, "NULL material pointer");
    NE_AssertPointer(pal, "NULL palette pointer");
    NE_AssertPointer(prepared, "NULL prepared texture pointer");

    if (tex == NULL || pal == NULL || prepared == NULL ||
        prepared->buffer == NULL || prepared->palette == NULL)
        return 0;

    NE_TextureFlags flags = NE_TEXGEN_TEXCOORD | NE_TEXTURE_WRAP_S
                          | NE_TEXTURE_WRAP_T;
    if (transpcolor)
        flags |= NE_TEXTURE_COLOR0_TRANSPARENT;

    int ret = NE_MaterialTexLoad(tex, NE_PAL256, prepared->texwidth,
                                 prepared->height, flags, prepared->buffer);
    if (ret == 0)
    {
        NE_DebugPrint("Error while loading texture");
        return 0;
    }

    // Load the full palette (unused entries are black), like Nitro Engine 0.7.0
    // did, in case some pixel uses an index past the colors stored in the BMP.
    ret = NE_PaletteLoad(pal, prepared->palette, prepared->numcolors, NE_PAL256);
    if (ret == 0)
    {
        NE_DebugPrint("Error while loading palette");
        return 0;
    }

    NE_MaterialSetPalette(tex, pal);
    cs_texture_size_save(tex, prepared->width, prepared->height);
    return 1;
}

/**
 * @brief Same as CS_MaterialTexLoadBMPtoRGB256(), but an 8 bpp BMP that only
 * uses the 16 first colors is loaded as a 16 colors texture (NE_PAL16): half
 * of the VRAM, and the conversion only needs a buffer of half of the size in
 * RAM (no 8 bpp copy). Used for the big images (map view: 256x177): online,
 * with a big map (Mirage), there wasn't enough free RAM for the 8 bpp copy.
 */
int CS_MaterialTexLoadBMPCompact(NE_Material *tex, NE_Palette *pal,
                                 const void *pointer, bool transpcolor)
{
    if (tex == NULL || pal == NULL || pointer == NULL)
        return 0;

    uintptr_t addr = (uintptr_t)pointer;
    if (addr < 0x02000000 || addr >= 0x03000000)
        return 0;

    const NE_BMPHeader *header = pointer;
    const NE_BMPInfoHeader *infoheader =
        (const void *)((const u8 *)header + sizeof(NE_BMPHeader));

    int sizex = (int32_t)infoheader->width;
    int sizey = (int32_t)infoheader->height;
    bool top_down = sizey < 0;
    if (top_down)
        sizey = -sizey;

    // Only uncompressed 8 bpp BMP files, the others use the normal loader
    if (header->type != 0x4D42 || infoheader->compression != 0 || infoheader->bits != 8 ||
        sizex <= 0 || sizey <= 0 || sizex > 1024 || sizey > 1024)
        return CS_MaterialTexLoadBMPtoRGB256(tex, pal, pointer, transpcolor);

    const u8 *palettedata = (const u8 *)infoheader + infoheader->size;
    const u8 *imagedata = (const u8 *)header + header->offset;
    int stride = ((sizex * 8 + 31) / 32) * 4;

    // Only the 16 first colors?
    for (int y = 0; y < sizey; y++)
    {
        const u8 *src = imagedata + y * stride;
        for (int x = 0; x < sizex; x++)
        {
            if (src[x] >= 16)
                return CS_MaterialTexLoadBMPtoRGB256(tex, pal, pointer, transpcolor);
        }
    }

    int texwidth = 8;
    while (texwidth < sizex)
        texwidth <<= 1;

    // 2 pixels per byte, the left pixel in the low 4 bits, padded with color 0
    u8 *buffer = calloc(texwidth / 2 * sizey, 1);
    if (buffer == NULL)
    {
        NE_DebugPrint("Not enough memory");
        return 0;
    }

    for (int y = 0; y < sizey; y++)
    {
        int srcy = top_down ? y : (sizey - y - 1);
        const u8 *src = imagedata + srcy * stride;
        u8 *dst = buffer + y * (texwidth / 2);
        for (int x = 0; x < sizex; x++)
        {
            if (x & 1)
                dst[x >> 1] |= src[x] << 4;
            else
                dst[x >> 1] |= src[x];
        }
    }

    u16 palette[16];
    int numcolors = infoheader->ncolors;
    if (numcolors <= 0 || numcolors > 16)
        numcolors = 16;
    for (int i = 0; i < 16; i++)
    {
        if (i < numcolors)
        {
            u8 b = palettedata[(i << 2) + 0];
            u8 g = palettedata[(i << 2) + 1];
            u8 r = palettedata[(i << 2) + 2];
            palette[i] = RGB15(r >> 3, g >> 3, b >> 3);
        }
        else
        {
            palette[i] = 0;
        }
    }

    NE_TextureFlags flags = NE_TEXGEN_TEXCOORD | NE_TEXTURE_WRAP_S | NE_TEXTURE_WRAP_T;
    if (transpcolor)
        flags |= NE_TEXTURE_COLOR0_TRANSPARENT;

    int ret = NE_MaterialTexLoad(tex, NE_PAL16, texwidth, sizey, flags, buffer);
    free(buffer);
    if (ret == 0)
    {
        NE_DebugPrint("Error while loading 16 colors texture");
        return 0;
    }

    ret = NE_PaletteLoad(pal, palette, 16, NE_PAL16);
    if (ret == 0)
    {
        NE_DebugPrint("Error while loading palette");
        return 0;
    }

    NE_MaterialSetPalette(tex, pal);
    cs_texture_size_save(tex, sizex, sizey);
    return 1;
}

/**
 * @brief Load a raw 16 colors texture (see tools/make_map_ui.py: u16 width,
 * height, texture width, 0, then 16 u16 palette entries, then the pixels, 2
 * per byte). The data is copied to VRAM directly, no buffer in RAM is needed.
 */
int CS_MaterialTexLoadRawPal16(NE_Material *tex, NE_Palette *pal,
                               const void *pointer, bool transpcolor)
{
    if (tex == NULL || pal == NULL || pointer == NULL)
        return 0;

    const u16 *header = pointer;
    int sizex = header[0];
    int sizey = header[1];
    int texwidth = header[2];
    const u16 *palette = header + 4;
    const u8 *pixels = (const u8 *)(palette + 16);

    NE_TextureFlags flags = NE_TEXGEN_TEXCOORD | NE_TEXTURE_WRAP_S | NE_TEXTURE_WRAP_T;
    if (transpcolor)
        flags |= NE_TEXTURE_COLOR0_TRANSPARENT;

    if (NE_MaterialTexLoad(tex, NE_PAL16, texwidth, sizey, flags, pixels) == 0)
    {
        NE_DebugPrint("Error while loading 16 colors texture");
        return 0;
    }
    if (NE_PaletteLoad(pal, palette, 16, NE_PAL16) == 0)
    {
        NE_DebugPrint("Error while loading palette");
        return 0;
    }

    NE_MaterialSetPalette(tex, pal);
    cs_texture_size_save(tex, sizex, sizey);
    return 1;
}

/**
 * @brief Load a raw 256x256 NE_PAL256 texture (see tools/make_map_textures.py:
 * 256 u16 palette entries, then the pixels, top row first). The data is copied
 * to VRAM directly, no buffer in RAM is needed.
 */
int CS_MaterialTexLoadRaw256(NE_Material *tex, NE_Palette *pal,
                             const void *pointer, bool transpcolor)
{
    if (tex == NULL || pal == NULL || pointer == NULL)
        return 0;

    CS_PreparedTexture prepared;
    prepared.palette = (u16 *)pointer;
    prepared.buffer = (u8 *)pointer + 256 * sizeof(u16);
    prepared.texwidth = 256;
    prepared.width = 256;
    prepared.height = 256;
    prepared.numcolors = 256;
    return CS_MaterialTexLoadPrepared(tex, pal, &prepared, transpcolor);
}

int CS_MaterialTexLoadBMPtoRGB256(NE_Material *tex, NE_Palette *pal,
                                  const void *pointer, bool transpcolor)
{
    NE_AssertPointer(tex, "NULL material pointer");
    NE_AssertPointer(pal, "NULL palette pointer");

    if (tex == NULL || pal == NULL)
        return 0;

    CS_PreparedTexture prepared;
    if (!CS_TexturePrepareBMP(&prepared, pointer))
        return 0;

    int ret = CS_MaterialTexLoadPrepared(tex, pal, &prepared, transpcolor);
    CS_TextureFreePrepared(&prepared);
    return ret;
}

// Deferred loading
// ----------------

#define CS_MAX_DEFERRED_TEXTURES 8

static struct {
    bool used;
    NE_Material **mat;
    NE_Palette **pal;
    bool transpcolor;
    CS_PreparedTexture prepared;
} cs_deferred[CS_MAX_DEFERRED_TEXTURES];

void CS_TextureCancelDeferred(NE_Material **mat)
{
    for (int i = 0; i < CS_MAX_DEFERRED_TEXTURES; i++)
    {
        if (cs_deferred[i].used && cs_deferred[i].mat == mat)
        {
            CS_TextureFreePrepared(&cs_deferred[i].prepared);
            cs_deferred[i].used = false;
        }
    }
}

static void cs_texture_replace(NE_Material **mat, NE_Palette **pal,
                               const CS_PreparedTexture *prepared,
                               bool transpcolor)
{
    // Don't use CS_MaterialDeleteSafe() here, it cancels deferred loads
    if (*mat != NULL)
        NE_MaterialDelete(*mat);
    if (*pal != NULL)
        NE_PaletteDelete(*pal);

    *mat = NE_MaterialCreate();
    *pal = NE_PaletteCreate();
    CS_MaterialTexLoadPrepared(*mat, *pal, prepared, transpcolor);
}

int CS_TextureLoadDeferred(NE_Material **mat, NE_Palette **pal,
                           const void *bmp, bool transpcolor)
{
    if (mat == NULL || pal == NULL)
        return 0;

    // A newer texture replaces a texture that wasn't loaded yet
    CS_TextureCancelDeferred(mat);

    int slot = -1;
    for (int i = 0; i < CS_MAX_DEFERRED_TEXTURES; i++)
    {
        if (!cs_deferred[i].used)
        {
            slot = i;
            break;
        }
    }

    CS_PreparedTexture prepared;
    if (!CS_TexturePrepareBMP(&prepared, bmp))
        return 0;

    if (slot == -1)
    {
        // Queue full (shouldn't happen): load now
        cs_texture_replace(mat, pal, &prepared, transpcolor);
        CS_TextureFreePrepared(&prepared);
        return 1;
    }

    // Empty material until the texture is loaded (not drawn)
    if (*mat == NULL)
        *mat = NE_MaterialCreate();

    cs_deferred[slot].used = true;
    cs_deferred[slot].mat = mat;
    cs_deferred[slot].pal = pal;
    cs_deferred[slot].transpcolor = transpcolor;
    cs_deferred[slot].prepared = prepared;
    return 1;
}

void CS_TextureApplyDeferred(void)
{
    // Frames waited for a good moment to load the textures
    static int waitedFrames = 0;

    for (int i = 0; i < CS_MAX_DEFERRED_TEXTURES; i++)
    {
        if (!cs_deferred[i].used)
            continue;

        // The 3D engine starts drawing the next frame at line 214. If the frame
        // was late (the wait for the vertical blank was skipped) or there is
        // no time left, load the rest on the next frame. If the game is too
        // slow for too long, load anyway (better a blink than no texture).
        int line = REG_VCOUNT;
        if ((line < 192 || line > 205) && waitedFrames < 30)
        {
            waitedFrames++;
            return;
        }
        waitedFrames = 0;

        CS_PreparedTexture prepared = cs_deferred[i].prepared;
        NE_Material **mat = cs_deferred[i].mat;
        NE_Palette **pal = cs_deferred[i].pal;
        bool transpcolor = cs_deferred[i].transpcolor;
        cs_deferred[i].used = false;

        cs_texture_replace(mat, pal, &prepared, transpcolor);
        CS_TextureFreePrepared(&prepared);
    }
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
    // Texture not loaded yet (deferred loading)
    if (mat == NULL || mat->texindex == NE_NO_TEXTURE)
        return;

    int w, h;
    CS_TextureGetRealSize(mat, &w, &h);
    NE_2DDrawTexturedQuadColorCanvas(x1, y1, x2, y2, z, 0, 0, w, h, mat,
                                     mat->color);
}

void CS_2DDrawTexturedQuadColor(s16 x1, s16 y1, s16 x2, s16 y2, s16 z,
                                const NE_Material *mat, u32 color)
{
    // Texture not loaded yet (deferred loading)
    if (mat == NULL || mat->texindex == NE_NO_TEXTURE)
        return;

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
