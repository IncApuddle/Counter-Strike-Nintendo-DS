// SPDX-License-Identifier: MIT
//
// Animated player models (DSM/DSA files made with md5_to_dsma).
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

#ifndef PLAYER_ANIM_H_
#define PLAYER_ANIM_H_

#include "main.h"

enum PlayerAnimState
{
    PLAYER_ANIM_IDLE = 0,
    PLAYER_ANIM_RUN,
    PLAYER_ANIM_JUMP,
    PLAYER_ANIM_FALL,
    PLAYER_ANIM_COUNT
};

// Scale of the animated model (f32). The model is 11.25 units tall after the
// conversion, and the feet must be 0.845 units under the player position (like
// the shadow).
#define PLAYER_ANIM_MODEL_SCALE 650

// The animated model looks at +Z, the old static model looked at -Z. This is
// added to the rotation of the model when it is drawn (512 = 360 degrees).
#define PLAYER_ANIM_MODEL_ROTATION 256

// Load the animations. Call it once after NE_InitDual3D().
void PlayerAnimInit(void);

// Create the model of a non-local player. It returns an animated model if
// animations are enabled (PlayerAnim setting), a static model otherwise.
NE_Model *PlayerAnimCreateModel(int playerIndex);

// Select the animation of every player from their movement and update the
// animations. Call it once per game frame.
void PlayerAnimUpdateAll(void);

// Draw the model of a player (adds PLAYER_ANIM_MODEL_ROTATION if needed).
void PlayerAnimDraw(Player *player);

#endif // PLAYER_ANIM_H_
