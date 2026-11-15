// SPDX-License-Identifier: MIT
//
// Animated player models (DSM/DSA files made with md5_to_dsma).
//
// The model and the animations come from Blender (MD5 export) and are converted
// with md5_to_dsma.py (see PORTING_BLOCKSDS.md). Every vertex is attached to one
// bone, so the CPU only has to calculate 15 bone matrices per model, the
// geometry is drawn by the GPU from a display list.
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

#include "playerAnim.h"
#include "player.h"
#include "party.h"

#include "obj_PlayerStatic_bin.h"
#include "gign_dsm_bin.h"
#include "gign_idle_dsa_bin.h"
#include "gign_run_dsa_bin.h"
#include "gign_jump_dsa_bin.h"
#include "gign_fall_dsa_bin.h"

// The animations were made at 24 FPS and the game runs at 60 FPS:
// 24 / 60 * 4096 = 1638 (f32 model frames per DS frame).
#define ANIM_SPEED 1638

// Number of DS frames used to blend two animations
#define ANIM_BLEND_FRAMES 6
#define ANIM_BLEND_STEP (4096 / ANIM_BLEND_FRAMES)

// Movement detection (f32 units per frame, 4096 = 1 unit)
#define MOVE_THRESHOLD 40      // ~0.01 units per frame. Bots walk at 0.087
#define AIR_THRESHOLD 80       // ~0.02 units per frame
#define TELEPORT_THRESHOLD 4096 // Bigger moves are spawns/respawns, not walking

// Frames to keep an animation after the movement stops. Network players don't
// move every frame, this avoids switching between idle and run all the time.
#define MOVE_KEEP_FRAMES 8
#define AIR_KEEP_FRAMES 4

static NE_Animation *Animations[PLAYER_ANIM_COUNT];
static bool AnimationsLoaded = false;

static const NE_AnimationType AnimTypes[PLAYER_ANIM_COUNT] = {
    NE_ANIM_LOOP,    // Idle
    NE_ANIM_LOOP,    // Run
    NE_ANIM_ONESHOT, // Jump
    NE_ANIM_LOOP,    // Fall
};

void PlayerAnimInit(void)
{
    if (AnimationsLoaded)
        return;

    const void *data[PLAYER_ANIM_COUNT] = {
        gign_idle_dsa_bin,
        gign_run_dsa_bin,
        gign_jump_dsa_bin,
        gign_fall_dsa_bin,
    };

    for (int i = 0; i < PLAYER_ANIM_COUNT; i++)
    {
        Animations[i] = NE_AnimationCreate();
        if (Animations[i] == NULL || NE_AnimationLoad(Animations[i], data[i]) == 0)
        {
            // If something fails, the game uses static models
            for (int j = 0; j <= i; j++)
            {
                if (Animations[j] != NULL)
                    NE_AnimationDelete(Animations[j]);
                Animations[j] = NULL;
            }
            return;
        }
    }

    AnimationsLoaded = true;
}

NE_Model *PlayerAnimCreateModel(int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];

    player->animState = PLAYER_ANIM_IDLE;
    player->animNext = -1;
    player->animBlend = 0;
    player->animMoveTimer = 0;
    player->animAirTimer = 0;
    player->animPrevX = 0;
    player->animPrevY = 0;
    player->animPrevZ = 0;

    if (PlayerAnim && AnimationsLoaded)
    {
        NE_Model *model = NE_ModelCreate(NE_Animated);
        if (model != NULL)
        {
            // The mesh isn't copied, all models use the same data in RAM
            if (NE_ModelLoadDSM(model, gign_dsm_bin))
            {
                NE_ModelSetAnimation(model, Animations[PLAYER_ANIM_IDLE]);
                NE_ModelAnimStart(model, AnimTypes[PLAYER_ANIM_IDLE], ANIM_SPEED);
                // Start bots at different frames so they don't move together
                NE_ModelAnimSetFrame(model, inttof32(playerIndex % 8));
                NE_ModelScaleI(model, PLAYER_ANIM_MODEL_SCALE,
                               PLAYER_ANIM_MODEL_SCALE, PLAYER_ANIM_MODEL_SCALE);
                return model;
            }
            NE_ModelDelete(model);
        }
    }

    // Static model (old behaviour)
    NE_Model *model = NE_ModelCreate(NE_Static);
    NE_ModelLoadStaticMesh(model, obj_PlayerStatic_bin);
    NE_ModelScaleI(model, 2048, 2090, 2048);
    return model;
}

// Start blending to a new animation
static void setAnimation(Player *player, int anim)
{
    NE_Model *model = player->PlayerModel;

    if (player->animNext == anim)
        return;

    if (player->animNext == -1 && player->animState == anim)
        return;

    // If another blend is in progress, finish it now
    if (player->animNext != -1)
    {
        NE_ModelAnimSecondaryClear(model, true);
        player->animState = player->animNext;
        player->animNext = -1;

        if (player->animState == anim)
            return;
    }

    NE_ModelSetAnimationSecondary(model, Animations[anim]);
    NE_ModelAnimSecondaryStart(model, AnimTypes[anim], ANIM_SPEED);
    player->animNext = anim;
    player->animBlend = 0;
}

static void updateBlend(Player *player)
{
    if (player->animNext == -1)
        return;

    player->animBlend += ANIM_BLEND_STEP;
    if (player->animBlend >= 4096)
    {
        // The new animation replaces the old one
        NE_ModelAnimSecondaryClear(player->PlayerModel, true);
        player->animState = player->animNext;
        player->animNext = -1;
        player->animBlend = 0;
    }
    else
    {
        NE_ModelAnimSecondarySetFactor(player->PlayerModel, player->animBlend);
    }
}

static void updatePlayer(Player *player)
{
    NE_Model *model = player->PlayerModel;

    int dx = model->x - player->animPrevX;
    int dy = model->y - player->animPrevY;
    int dz = model->z - player->animPrevZ;

    player->animPrevX = model->x;
    player->animPrevY = model->y;
    player->animPrevZ = model->z;

    int adx = abs(dx), ady = abs(dy), adz = abs(dz);
    int horizontal = adx > adz ? adx : adz; // Fast approximation of the length

    // Spawn or respawn: ignore this movement
    if (horizontal > TELEPORT_THRESHOLD || ady > TELEPORT_THRESHOLD)
    {
        updateBlend(player);
        return;
    }

    if (horizontal > MOVE_THRESHOLD)
        player->animMoveTimer = MOVE_KEEP_FRAMES;
    else if (player->animMoveTimer > 0)
        player->animMoveTimer--;

    // Vertical movement that isn't caused by walking on stairs or slopes
    int target;
    if (ady > AIR_THRESHOLD && ady * 4 > horizontal * 3)
    {
        player->animAirTimer = AIR_KEEP_FRAMES;
        target = (dy > 0) ? PLAYER_ANIM_JUMP : PLAYER_ANIM_FALL;
    }
    else if (player->animAirTimer > 0)
    {
        player->animAirTimer--;
        // Keep the current air animation
        int current = (player->animNext != -1) ? player->animNext : player->animState;
        if (current == PLAYER_ANIM_JUMP || current == PLAYER_ANIM_FALL)
            target = current;
        else
            target = PLAYER_ANIM_FALL;
    }
    else if (player->animMoveTimer > 0)
    {
        target = PLAYER_ANIM_RUN;
    }
    else
    {
        target = PLAYER_ANIM_IDLE;
    }

    setAnimation(player, target);
    updateBlend(player);
}

void PlayerAnimUpdateAll(void)
{
    if (!AnimationsLoaded)
        return;

    bool paused = (roundState == WAIT_START);

    for (int i = 1; i < MaxPlayer; i++)
    {
        Player *player = &AllPlayers[i];

        if (player->Id == UNUSED || player->IsDead)
            continue;

        if (!CS_ModelIsAnimated(player->PlayerModel))
            continue;

        if (paused)
        {
            // Players can't move before the round starts
            player->animPrevX = player->PlayerModel->x;
            player->animPrevY = player->PlayerModel->y;
            player->animPrevZ = player->PlayerModel->z;
            setAnimation(player, PLAYER_ANIM_IDLE);
            updateBlend(player);
            continue;
        }

        updatePlayer(player);
    }

    NE_ModelAnimateAll();
}

void PlayerAnimDraw(Player *player)
{
    NE_Model *model = player->PlayerModel;

    if (CS_ModelIsAnimated(model))
    {
        int ry = model->ry;
        model->ry = ry + PLAYER_ANIM_MODEL_ROTATION;
        NE_ModelDraw(model);
        model->ry = ry;
    }
    else
    {
        NE_ModelDraw(model);
    }
}
