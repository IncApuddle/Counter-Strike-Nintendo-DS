// SPDX-License-Identifier: MIT
//
// Copyright (c) 2026, Counter-Strike DS contributors
//
// Guns dropped on the floor.
//
// Only primary guns and pistols can be dropped (no knife, grenades, kit), in
// Competitive and Casual modes. A player takes a gun automatically when he
// walks on it and the slot of this gun is free.
//
// Offline, the game manages everything. Online, the server is the authority:
// the game only sends the drop request (DROPGUN) and shows the guns that the
// server sends (DROPGUN / REMOVEGUN messages, see network.c).
//
// All guns are drawn with the same texture: an atlas of the small icons of
// the guns (data/dropped_guns.bin, made by tools/make_dropped_guns_atlas.py).
// The atlas is loaded once at the start of the game (16 KB of VRAM).

#include "main.h"
#include "droppedGuns.h"
#include "dropped_guns_bin.h"
#include "gun.h"
#include "player.h"
#include "party.h"
#include "map.h"
#include "collisions.h"
#include "sounds.h"
#include "network.h"
#include "ui.h"
#include "localhost.h"
#include "tutorial.h"

// Atlas format (see tools/make_dropped_guns_atlas.py)
#define ATLAS_WIDTH 256
#define ATLAS_HEIGHT 128
#define ATLAS_CELL 32
#define ATLAS_COLUMNS 8
#define ATLAS_PALETTE_SIZE 32 // 16 colors
#define ATLAS_FIRST_GUN 1     // Cell 0 is the Glock-18 (AllGuns[1])
#define ATLAS_LAST_GUN 24     // G3SG1

// Size of the icon in the world and height above the floor
#define ICON_HALF_SIZE 0.35f
#define ICON_HEIGHT 0.45f

// The drop button throws the gun in front of the player
#define THROW_DISTANCE 1.5f
#define THROW_WALL_MARGIN 0.4f

// Distance to take a gun
#define PICKUP_DISTANCE 0.8f
#define PICKUP_HEIGHT 1.6f

DroppedGun droppedGuns[MAX_DROPPED_GUNS];
bool SendDropGun = false;

static NE_Material *droppedGunsMaterial = NULL;
static NE_Palette *droppedGunsPalette = NULL;
static int droppedGunsAge = 0;
static int bobbingFrame = 0;

// Field of view of the camera, computed in Draw3DScene() (draw3d.c)
extern int t1x, t1z, t2x, t2z;

/**
 * @brief Load the texture of the dropped guns (call once)
 *
 */
void initDroppedGuns(void)
{
    clearDroppedGuns();

    if (droppedGunsMaterial != NULL)
        return;

    droppedGunsMaterial = NE_MaterialCreate();
    droppedGunsPalette = NE_PaletteCreate();

    const u8 *data = (const u8 *)dropped_guns_bin;
    int ret = NE_MaterialTexLoad(droppedGunsMaterial, NE_PAL16, ATLAS_WIDTH, ATLAS_HEIGHT,
                                 NE_TEXGEN_TEXCOORD | NE_TEXTURE_COLOR0_TRANSPARENT,
                                 data + ATLAS_PALETTE_SIZE);
    if (ret == 0)
    {
        // Not enough VRAM: the guns work but they aren't drawn
        NE_MaterialDelete(droppedGunsMaterial);
        NE_PaletteDelete(droppedGunsPalette);
        droppedGunsMaterial = NULL;
        droppedGunsPalette = NULL;
        return;
    }

    NE_PaletteLoad(droppedGunsPalette, (const u16 *)data, 16, NE_PAL16);
    NE_MaterialSetPalette(droppedGunsMaterial, droppedGunsPalette);
}

/**
 * @brief Remove all dropped guns
 *
 */
void clearDroppedGuns(void)
{
    for (int i = 0; i < MAX_DROPPED_GUNS; i++)
        droppedGuns[i].used = false;
}

/**
 * @brief Can guns be dropped in the current mode? (Competitive and Casual)
 *
 */
bool canDropGunsInCurrentMode(void)
{
    return currentPartyMode == 0 || currentPartyMode == 1;
}

/**
 * @brief Is the gun a gun that can be dropped? (pistols and primary guns)
 *
 */
static bool isDroppableGun(int gunId)
{
    return gunId >= ATLAS_FIRST_GUN && gunId <= ATLAS_LAST_GUN;
}

/**
 * @brief Inventory slot of a gun (1 pistols, 2 primary guns)
 *
 */
static int getGunSlot(int gunId)
{
    return AllGuns[gunId].gunCategory == PISTOLS ? 1 : 2;
}

/**
 * @brief Find the occlusion zone of a position
 *
 */
static int findZone(float x, float z)
{
    Map *map = &allMaps[currentMap];
    if (map->AllZones == NULL)
        return -1;

    for (int i = 0; i < map->zonesCount; i++)
    {
        CollisionBox2D *box = &map->AllZones[i].collisionBox;
        if (x <= box->BoxXRangeA && x >= box->BoxXRangeB && z <= box->BoxZRangeA && z >= box->BoxZRangeB)
            return i;
    }
    return -1;
}

/**
 * @brief Add a gun on the floor. If there are too many guns, the oldest one
 * is removed.
 *
 * @return Index of the gun in droppedGuns, -1 if the gun can't be dropped
 */
int addDroppedGun(int gunId, float x, float y, float z, int ammoCount, int totalAmmo, int ownerIndex, int netId)
{
    if (!isDroppableGun(gunId))
        return -1;

    int slot = -1;
    int oldestAge = 0;
    for (int i = 0; i < MAX_DROPPED_GUNS; i++)
    {
        if (!droppedGuns[i].used)
        {
            slot = i;
            break;
        }
        // Remove the oldest gun if there is no free slot
        if (slot == -1 || droppedGuns[i].age < oldestAge)
        {
            slot = i;
            oldestAge = droppedGuns[i].age;
        }
    }

    DroppedGun *gun = &droppedGuns[slot];
    gun->used = true;
    gun->netId = netId;
    gun->gunId = gunId;
    gun->ammoCount = ammoCount;
    gun->totalAmmo = totalAmmo;
    gun->x = x;
    gun->y = y;
    gun->z = z;
    gun->zone = findZone(x, z);
    gun->ownerIndex = ownerIndex;
    gun->ownerDelay = DROPPED_GUN_OWNER_DELAY;
    gun->age = droppedGunsAge++;
    return slot;
}

/**
 * @brief Remove a gun sent by the server (-1 removes all the guns)
 *
 */
void removeDroppedGunByNetId(int netId)
{
    for (int i = 0; i < MAX_DROPPED_GUNS; i++)
    {
        if (droppedGuns[i].used && (netId == -1 || droppedGuns[i].netId == netId))
            droppedGuns[i].used = false;
    }
}

/**
 * @brief Select the best gun after a drop (primary gun, pistol, then knife)
 *
 */
static void selectBestGun(int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    int newSlot = 0;
    if (player->AllGunsInInventory[2] != EMPTY)
        newSlot = 2;
    else if (player->AllGunsInInventory[1] != EMPTY)
        newSlot = 1;

    if (playerIndex == 0)
    {
        DisableAim();
        setSelectedGunInInventory(0, newSlot);
    }
    else
    {
        player->currentGunInInventory = newSlot;
        StopReloading(playerIndex);
    }
}

/**
 * @brief Distance to the nearest wall in a horizontal direction, from a
 * point at 0.5 above the floor of the player
 *
 * @param dirX Direction (normalized)
 * @param dirZ Direction (normalized)
 */
static float getWallDistance(Player *player, float dirX, float dirZ)
{
    // Not 0: 1 / 0 would give infinite values (an almost horizontal ray)
    float dx = fabsf(dirX) < 0.001f ? 0.001f : dirX;
    float dz = fabsf(dirZ) < 0.001f ? 0.001f : dirZ;

    Vector3 dirfrac;
    dirfrac.x = 1.0f / dx;
    dirfrac.y = 1.0f / 0.001f;
    dirfrac.z = 1.0f / dz;

    // Walls are * 8192 (see Raycast() in raycast.c)
    Vector3Int startPosition;
    startPosition.x = player->position.x * 8192;
    startPosition.y = (player->position.y - 0.845f + 0.5f) * 8192;
    startPosition.z = player->position.z * 8192;

    float t = THROW_DISTANCE * 2 * 8192;
    Wall *walls = getMapWalls();
    int count = allMaps[currentMap].CollisionsCount;
    for (int i = 0; i < count; i++)
    {
        Vector3Int corner1, corner2;
        corner1.x = walls[i].WallCollisionBox.BoxXRangeA;
        corner1.y = walls[i].WallCollisionBox.BoxYRangeA;
        corner1.z = walls[i].WallCollisionBox.BoxZRangeA;
        corner2.x = walls[i].WallCollisionBox.BoxXRangeB;
        corner2.y = walls[i].WallCollisionBox.BoxYRangeB;
        corner2.z = walls[i].WallCollisionBox.BoxZRangeB;
        getHitDistance(corner1, corner2, dirfrac, startPosition, &t);
    }

    float distance = t / 8192.0f;
    return distance < 0 ? 0 : distance;
}

/**
 * @brief Remove a gun from the inventory of a player and put it on the floor
 * (offline only)
 *
 * @param throwGun True: in front of the player (drop button), false: at the
 * position of the player (death)
 */
static void dropGunFromSlot(int playerIndex, int inventorySlot, bool throwGun)
{
    Player *player = &AllPlayers[playerIndex];
    int gunId = player->AllGunsInInventory[inventorySlot];
    if (!isDroppableGun(gunId))
        return;

    float x = player->position.x;
    float z = player->position.z;
    if (throwGun)
    {
        // Direction of the player (same as the forward movement)
        float angle = player->Angle / 512.0f * M_TWOPI;
        float dirX = -sinf(angle);
        float dirZ = -cosf(angle);

        // Not through the walls
        float distance = getWallDistance(player, dirX, dirZ) - THROW_WALL_MARGIN;
        if (distance > THROW_DISTANCE)
            distance = THROW_DISTANCE;
        if (distance > 0)
        {
            x += dirX * distance;
            z += dirZ * distance;
        }
    }

    AmmoMagazine *ammo = &player->AllAmmoMagazine[inventorySlot - 1];
    addDroppedGun(gunId, x, player->position.y - 0.845, z,
                  ammo->AmmoCount, ammo->TotalAmmoCount, playerIndex, -1);

    player->AllGunsInInventory[inventorySlot] = EMPTY;
}

/**
 * @brief Can the player drop the gun in his hands?
 *
 */
bool canPlayerDropCurrentGun(int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    if (!canDropGunsInCurrentMode() || isInTutorial || player->IsDead || player->Team == SPECTATOR || player->isPlantingBomb)
        return false;

    int slot = player->currentGunInInventory;
    if (slot != 1 && slot != 2)
        return false;

    return isDroppableGun(player->AllGunsInInventory[slot]);
}

/**
 * @brief Drop the gun in the hands of the player (drop button)
 *
 */
void dropCurrentGun(int playerIndex)
{
    if (!canPlayerDropCurrentGun(playerIndex))
        return;

    // Online, the server drops the gun and sends the new inventory
    if (Connection != OFFLINE)
    {
        if (playerIndex == 0)
        {
            DisableAim();
            StopReloading(0);
            SendDropGun = true;
        }
        return;
    }

    dropGunFromSlot(playerIndex, AllPlayers[playerIndex].currentGunInInventory, true);
    selectBestGun(playerIndex);
}

/**
 * @brief Drop the primary gun (or the pistol if there is no primary gun) of
 * a dead player (offline only)
 *
 */
void dropGunOnDeath(int playerIndex)
{
    if (Connection != OFFLINE || !canDropGunsInCurrentMode())
        return;

    Player *player = &AllPlayers[playerIndex];
    if (isDroppableGun(player->AllGunsInInventory[2]))
        dropGunFromSlot(playerIndex, 2, false);
    else if (isDroppableGun(player->AllGunsInInventory[1]))
        dropGunFromSlot(playerIndex, 1, false);
    else
        return;

    // Never keep an empty slot in the hands (AllGuns[-1] doesn't exist)
    if (getPlayerCurrentGunIndex(player) == EMPTY)
    {
        if (player->AllGunsInInventory[2] != EMPTY)
            player->currentGunInInventory = 2;
        else if (player->AllGunsInInventory[1] != EMPTY)
            player->currentGunInInventory = 1;
        else
            player->currentGunInInventory = 0;

        if (playerIndex == CurrentCameraPlayer)
            UpdateGunTexture();
    }
}

/**
 * @brief Give a dropped gun to a player (offline only)
 *
 */
static void takeDroppedGun(int playerIndex, DroppedGun *gun)
{
    Player *player = &AllPlayers[playerIndex];
    int slot = getGunSlot(gun->gunId);

    player->AllGunsInInventory[slot] = gun->gunId;
    player->AllAmmoMagazine[slot - 1].AmmoCount = gun->ammoCount;
    player->AllAmmoMagazine[slot - 1].TotalAmmoCount = gun->totalAmmo;
    gun->used = false;

    if (playerIndex == 0)
    {
        // Take a primary gun in the hands, a pistol only if the player has the knife
        if (slot == 2 || player->currentGunInInventory == 0)
        {
            DisableAim();
            setSelectedGunInInventory(0, slot);
        }
        PlayBasicSound(SFX_MOLOTOV_DETONATE);
    }
    else
    {
        // Local multiplayer: pick up sound for the client
        localHostOnGunPickup(playerIndex);

        // Bots take the best gun
        if (slot == 2 || player->currentGunInInventory == 0)
        {
            player->currentGunInInventory = slot;
            StopReloading(playerIndex);
        }
    }
}

/**
 * @brief Take the guns on the floor (offline) and update the timers
 *
 */
void updateDroppedGuns(void)
{
    bobbingFrame++;

    for (int i = 0; i < MAX_DROPPED_GUNS; i++)
    {
        if (droppedGuns[i].used && droppedGuns[i].ownerDelay > 0)
            droppedGuns[i].ownerDelay--;
    }

    // Online, the server gives the guns
    if (Connection != OFFLINE)
        return;

    for (int playerIndex = 0; playerIndex < MaxPlayer; playerIndex++)
    {
        Player *player = &AllPlayers[playerIndex];
        if (player->Id == UNUSED || player->IsDead || player->Team == SPECTATOR)
            continue;

        for (int i = 0; i < MAX_DROPPED_GUNS; i++)
        {
            DroppedGun *gun = &droppedGuns[i];
            if (!gun->used)
                continue;

            // The player who dropped the gun doesn't take it back at once
            if (gun->ownerIndex == playerIndex && gun->ownerDelay > 0)
                continue;

            // Only if the slot is free
            if (player->AllGunsInInventory[getGunSlot(gun->gunId)] != EMPTY)
                continue;

            float floorY = player->position.y - 0.845;
            if (fabsf(player->position.x - gun->x) > PICKUP_DISTANCE ||
                fabsf(player->position.z - gun->z) > PICKUP_DISTANCE ||
                fabsf(floorY - gun->y) > PICKUP_HEIGHT)
                continue;

            takeDroppedGun(playerIndex, gun);
        }
    }
}

/**
 * @brief Is the gun visible from the camera? (same checks as the players)
 *
 */
static bool isDroppedGunVisible(DroppedGun *gun, Player *cameraPlayer)
{
    Map *map = &allMaps[currentMap];

    int gunX = floattof32(gun->x);
    int gunZ = floattof32(gun->z);

    if (!PointInTriangleInt(gunX, gunZ, cameraPlayer->PlayerModel->x, cameraPlayer->PlayerModel->z, t1x, t1z, t2x, t2z))
        return false;

    // Unknown zone (outside of the zones of the map): draw it
    if (gun->zone < 0 || gun->zone >= map->zonesCount)
        return true;

    // The camera must be in a part of the map visible from the zone of the gun
    for (int i = 0; i < map->AllZones[gun->zone].ZoneCount; i++)
    {
        int part = map->AllZones[gun->zone].visibleMapPart[i];
        if (checkZoneForOcclusion(&map->AllOcclusionZone[part], cameraPlayer->PlayerModel->x, cameraPlayer->PlayerModel->z))
            return true;
    }
    return false;
}

/**
 * @brief Draw the dropped guns (the icon of the gun, facing the camera)
 *
 */
void drawDroppedGuns(void)
{
    if (droppedGunsMaterial == NULL || droppedGunsMaterial->texindex == NE_NO_TEXTURE)
        return;

    Player *cameraPlayer = &AllPlayers[CurrentCameraPlayer];
    if (cameraPlayer->PlayerModel == NULL)
        return;

    // Small up and down movement to see the guns better
    float bobbing = sinf(bobbingFrame * 0.05f) * 0.05f;

    bool materialSet = false;
    for (int i = 0; i < MAX_DROPPED_GUNS; i++)
    {
        DroppedGun *gun = &droppedGuns[i];
        if (!gun->used || !isDroppedGunVisible(gun, cameraPlayer))
            continue;

        if (!materialSet)
        {
            // No lights: the icon is drawn with its real colors
            NE_PolyFormat(31, 0, (NE_LightEnum)0, NE_CULL_NONE, NE_MODULATION);
            NE_MaterialUse(droppedGunsMaterial);
            NE_PolyColor(RGB15(26, 26, 26));
            materialSet = true;
        }

        // Turn the icon to the camera
        float dx = gun->x - cameraPlayer->position.x;
        float dz = gun->z - cameraPlayer->position.z;
        int angle = (int)(atan2f(dx, dz) * (32768.0f / (2.0f * M_PI))) & 0x7FFF;

        // Texture coordinates of the gun in the atlas
        int cell = gun->gunId - ATLAS_FIRST_GUN;
        int u1 = (cell % ATLAS_COLUMNS) * ATLAS_CELL;
        int v1 = (cell / ATLAS_COLUMNS) * ATLAS_CELL;
        int u2 = u1 + ATLAS_CELL;
        int v2 = v1 + ATLAS_CELL;

        int size = floattov16(ICON_HALF_SIZE);

        MATRIX_PUSH = 0;
        glTranslatef32(floattof32(gun->x), floattof32(gun->y + ICON_HEIGHT + bobbing), floattof32(gun->z));
        glRotateYi(angle);

        // The left of the image is on +X: after the rotation, it is on the
        // left of the camera
        NE_PolyBegin(GL_QUADS);
        NE_PolyTexCoord(u1, v1);
        NE_PolyVertexI(size, size, 0);
        NE_PolyTexCoord(u1, v2);
        NE_PolyVertexI(size, -size, 0);
        NE_PolyTexCoord(u2, v2);
        NE_PolyVertexI(-size, -size, 0);
        NE_PolyTexCoord(u2, v1);
        NE_PolyVertexI(-size, size, 0);
        NE_PolyEnd();

        MATRIX_POP = 1;
    }

    if (materialSet)
        NE_PolyFormat(31, 0, NE_LIGHT_0, NE_CULL_BACK, NE_MODULATION);
}
