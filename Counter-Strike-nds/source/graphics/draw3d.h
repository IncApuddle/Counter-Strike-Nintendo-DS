// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021-2022, Fewnity - Grégory Machefer
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

#ifndef DRAW3D_H_ /* Include guard */
#define DRAW3D_H_

void initGraphics();
void Draw3DScene(void);
void DrawPlayers();
void Draw3DSceneNotInGame(void);
void createPlayerShadow();
void SetTwoScreenMode(bool value);

extern NE_Palette *Palettes[18];
extern NE_Material *TopScreenSpritesMaterials[6];
extern NE_Material *BottomScreenSpritesMaterials[9];

extern int MapImgToLoad;
extern int CurrentTexture;

void MapImgToLoadFunc();
void TextureLoadOfMap();
void CheckCurTextur();

#endif // DRAW3D_H_