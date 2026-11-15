// SPDX-License-Identifier: MIT
//
// Copyright (c) 2026, Counter-Strike DS contributors
//
// Helpers used to port Counter-Strike DS from the modified Nitro Engine 0.7.0
// to the official Nitro Engine 0.15.x.

#ifndef CS_COMPAT_H_
#define CS_COMPAT_H_

#include <math.h>

#include <NEMain.h>

// Not every C library defines M_TWOPI (newlib does, others may not)
#ifndef M_TWOPI
#define M_TWOPI (M_PI * 2.0)
#endif

// Nitro Engine 0.7.0 had separate "diffuse", "ambient", "specular" and
// "emission" fields in NE_Material. Nitro Engine 0.15.x packs them in the same
// format as the GPU registers:
//
//   diffuse_ambient   = diffuse  | (vtxcolor << 15)     | (ambient  << 16)
//   specular_emission = specular | (useshininess << 15) | (emission << 16)
//
// These helpers change one color and keep everything else.

static inline void CS_MaterialSetDiffuse(NE_Material *mat, u32 color)
{
    mat->diffuse_ambient = (mat->diffuse_ambient & ~0x7FFFu) | (color & 0x7FFFu);
}

static inline void CS_MaterialSetAmbient(NE_Material *mat, u32 color)
{
    mat->diffuse_ambient = (mat->diffuse_ambient & ~(0x7FFFu << 16))
                         | ((color & 0x7FFFu) << 16);
}

static inline void CS_MaterialSetSpecular(NE_Material *mat, u32 color)
{
    mat->specular_emission = (mat->specular_emission & ~0x7FFFu) | (color & 0x7FFFu);
}

static inline void CS_MaterialSetEmission(NE_Material *mat, u32 color)
{
    mat->specular_emission = (mat->specular_emission & ~(0x7FFFu << 16))
                           | ((color & 0x7FFFu) << 16);
}

// Animation functions of Nitro Engine 0.15 crash if they are used with a
// static model, so check the model type before using them.
static inline bool CS_ModelIsAnimated(const NE_Model *model)
{
    return (model != NULL) && (model->modeltype == NE_Animated);
}

// Same as NE_MaterialTexLoadBMPtoRGB256(), but the texture repeats (wraps) in
// both axes. The modified Nitro Engine used by Counter-Strike DS enabled this
// for every BMP texture, and the maps depend on it.
//
// Supports uncompressed 4 bpp and 8 bpp BMP files. Returns 1 on success.
int CS_MaterialTexLoadBMPtoRGB256(NE_Material *tex, NE_Palette *pal,
                                  const void *pointer, bool transpcolor);

// Textures loaded with CS_MaterialTexLoadBMPtoRGB256() may be wider than the
// image (the width is padded to a power of 2). These functions draw only the
// real image, like the old modified engine did. Use them instead of
// NE_2DDrawTexturedQuad(), NE_2DDrawTexturedQuadColor() and
// NE_SpriteSetMaterial().
void CS_TextureGetRealSize(const NE_Material *mat, int *w, int *h);
void CS_2DDrawTexturedQuad(s16 x1, s16 y1, s16 x2, s16 y2, s16 z,
                           const NE_Material *mat);
void CS_2DDrawTexturedQuadColor(s16 x1, s16 y1, s16 x2, s16 y2, s16 z,
                                const NE_Material *mat, u32 color);
void CS_SpriteSetMaterial(NE_Sprite *sprite, NE_Material *mat);

// Delete a material or a palette only if it exists, and set the pointer to
// NULL. Nitro Engine 0.15 crashes with NULL pointers, and the game deleted some
// of them twice or before creating them.
static inline void CS_MaterialDeleteSafe(NE_Material **mat)
{
    if (*mat != NULL)
        NE_MaterialDelete(*mat);
    *mat = NULL;
}

static inline void CS_PaletteDeleteSafe(NE_Palette **pal)
{
    if (*pal != NULL)
        NE_PaletteDelete(*pal);
    *pal = NULL;
}

#endif // CS_COMPAT_H_
