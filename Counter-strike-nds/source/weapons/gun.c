// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021-2022, Fewnity - Grégory Machefer
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

//#include "main.h"
#include "gun.h"
#include "data/gun_data.h"
#include "sounds.h"
#include "ui.h"
#include "party.h"
#include "network.h"
#include "equipment.h"
#include "player.h"
#include "draw3d.h"

// Loaded guns data
Gun AllGuns[GunCount];

// Cheapest gun of each category for counters terrorists
int cheapestGunsCostCounter[4];
// Cheapest gun of each category for terrorists
int cheapestGunsCostTerrorists[4];
// Gun count for each category for counters terrorists
int gunCountCounter[4];
// Gun count for each category for terrorists
int gunCountTerrorists[4];

/**
 * @brief Load guns data
 *
 */
void initGuns()
{
    LoadGuns();
    // Reset cheapest guns cost list
    for (int i = 0; i < 4; i++)
    {
        cheapestGunsCostTerrorists[i] = 9999;
        cheapestGunsCostCounter[i] = 9999;
    }

    // Get cheapest and gun count for each category
    for (int i = 1; i < GunCount; i++)
    {
        Gun *gun = &AllGuns[i];

        if (gun->team == COUNTERTERRORISTS || gun->team == BOTH)
        {
            gunCountCounter[gun->gunCategory]++;
            // If the current gun price is lower than the cheapest gun
            if (gun->Price < cheapestGunsCostCounter[gun->gunCategory])
            {
                // Set the new cheapest gun price
                cheapestGunsCostCounter[gun->gunCategory] = gun->Price;
            }
        }
        if (gun->team == TERRORISTS || gun->team == BOTH)
        {
            gunCountTerrorists[gun->gunCategory]++;
            // If the current gun price is lower than the cheapest gun
            if (gun->Price < cheapestGunsCostTerrorists[gun->gunCategory])
            {
                // Set the new cheapest gun price
                cheapestGunsCostTerrorists[gun->gunCategory] = gun->Price;
            }
        }
    }
}

/**
 * @brief Reload the current gun of a player
 *
 * @param playerIndex Index of the player to reload
 */
void ReloadGun(int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    // Check if the weapon in the player hands is a gun
    if (player->currentGunInInventory == 1 || player->currentGunInInventory == 2)
    {
        int ammoIndex = player->currentGunInInventory - 1;
        // Get how many ammo are missing
        int missingAmmoCount = getPlayerCurrentGun(player).MagazineCapacity - player->AllAmmoMagazine[ammoIndex].AmmoCount;
        // Shotguns: one shell at a time
        if (getPlayerCurrentGunIndex(player) < GunCount && getPlayerCurrentGun(player).reloadOneByOne && missingAmmoCount > 1)
            missingAmmoCount = 1;
        if (missingAmmoCount <= 0)
        {
            // Magazine already full
        }
        else if (missingAmmoCount < player->AllAmmoMagazine[ammoIndex].TotalAmmoCount)
        {
            player->AllAmmoMagazine[ammoIndex].AmmoCount += missingAmmoCount;
            if (!allPartyModes[currentPartyMode].infiniteGunAmmo)
                player->AllAmmoMagazine[ammoIndex].TotalAmmoCount -= missingAmmoCount;
        }
        else
        {
            player->AllAmmoMagazine[ammoIndex].AmmoCount += player->AllAmmoMagazine[ammoIndex].TotalAmmoCount;
            player->AllAmmoMagazine[ammoIndex].TotalAmmoCount = 0;
        }
    }
    if (player == localPlayer)
    {
        SendReloaded = true;
    }
}

/**
 * @brief Set the aim to the local player
 *
 */
void SetAim()
{
    doubleTapTimer = 0;
    if (getPlayerCurrentGunIndex(localPlayer) < GunCount && getPlayerCurrentGun(localPlayer).scopeId != -1)
    {
        if (CurrentScopeLevel < AllScopeLevels[getPlayerCurrentGun(localPlayer).scopeId].scopeCount)
        {
            // Change fov
            CurrentScopeLevel++;
            NE_SetFov(AllScopeLevels[getPlayerCurrentGun(localPlayer).scopeId].fov[CurrentScopeLevel - 1]);
            // Make scope sound
            PlayBasicSound(SFX_SCOPE);
        }
        else
        {
            DisableAim();
        }
    }
}

/**
 * @brief Apply gun y offset
 *
 */
void applyGunAnimation()
{
    // Set gun sprite offset position
    if (yOffset < 0.1)
        yOffset += (rand() % 2 + 2) / 100.0;
    else if (yOffset < 0.16)
        yOffset += (rand() % 2 + 2) / 150.0;
    else
        yOffset += (rand() % 2 + 2) / 200.0;
}

/**
 * @brief Disable aiming
 *
 */
void DisableAim()
{
    CurrentScopeLevel = 0;
    // Set default fov
    // TODO make const value
    NE_SetFov(70);
}

int getPlayerCurrentGunIndex(Player *player)
{
    return player->AllGunsInInventory[player->currentGunInInventory];
}

Gun getPlayerCurrentGun(Player *player)
{
    return AllGuns[player->AllGunsInInventory[player->currentGunInInventory]];
}

/**
 * @brief Buy a random gun of a category for a player
 *
 * @param gunCategory Gun category to buy
 * @param playerIndex Index of the player to buy the gun for
 */
void findGun(int gunCategory, int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    int gunFound = -1;
    while (gunFound == -1)
    {
        int random2;
        // Get a random index from the gun count of the category
        if (player->Team == COUNTERTERRORISTS)
        {
            random2 = rand() % gunCountCounter[gunCategory];
        }
        else
        {
            random2 = rand() % gunCountTerrorists[gunCategory];
        }

        int count = 0;
        for (int i2 = 0; i2 < GunCount; i2++)
        {
            if (AllGuns[i2].gunCategory == gunCategory && (AllGuns[i2].team == player->Team || AllGuns[i2].team == BOTH))
            {
                // if the player has enough money
                if (count == random2 && AllGuns[i2].Price <= player->Money)
                {
                    // Return the gun index
                    gunFound = i2;
                    break;
                }
                else
                    count++;
            }
        }
    }

    // Reduce money
    reducePlayerMoney(playerIndex, AllGuns[gunFound].Price);

    // Set the gun in the inventory
    if (gunCategory == 0)
    {
        AllPlayers[playerIndex].AllGunsInInventory[1] = gunFound;
        AllPlayers[playerIndex].currentGunInInventory = 1;
    }
    else
    {
        AllPlayers[playerIndex].AllGunsInInventory[2] = gunFound;
        AllPlayers[playerIndex].currentGunInInventory = 2;
    }

    // Reset new gun ammo
    ResetGunsAmmo(playerIndex);

    // Set the gun texture at screen
    if (playerIndex == 0)
    {
        UpdateGunTexture();
    }
}

/**
 * @brief Reset gun ammo (set ammo to max capacity)
 *
 * @param playerIndex Index of the player to reset the ammo for
 * @param inventoryIndex Index of the inventory to reset the ammo for
 */
void ResetGunAmmo(int playerIndex, int inventoryIndex)
{
    if (inventoryIndex < 1 || inventoryIndex > 2)
        return;

    Player *player = &AllPlayers[playerIndex];

    // Set the ammo to max capacity
    int ammoIndex = inventoryIndex - 1;
    player->AllAmmoMagazine[ammoIndex].AmmoCount = AllGuns[AllPlayers[playerIndex].AllGunsInInventory[inventoryIndex]].MagazineCapacity;
    player->AllAmmoMagazine[ammoIndex].TotalAmmoCount = AllGuns[AllPlayers[playerIndex].AllGunsInInventory[inventoryIndex]].MaxAmmoCount;
}

/**
 * @brief
 *
 * @param playerIndex
 */
void ResetGunsAmmo(int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    for (int ammoIndex = 0; ammoIndex < 2; ammoIndex++)
    {
        int inventoryIndex = ammoIndex + 1;
        if (player->AllGunsInInventory[inventoryIndex] != -1)
        {
            // Set the ammo to max capacity
            player->AllAmmoMagazine[ammoIndex].AmmoCount = AllGuns[AllPlayers[playerIndex].AllGunsInInventory[inventoryIndex]].MagazineCapacity;
            player->AllAmmoMagazine[ammoIndex].TotalAmmoCount = AllGuns[AllPlayers[playerIndex].AllGunsInInventory[inventoryIndex]].MaxAmmoCount;
        }
    }
}

/**
 * @brief Set gun recoil
 *
 * @param player Player pointer to set the recoil for
 */
void setGunRecoil(Player *player)
{
    // If the gun is a dual gun, swap the recoil
    if (getPlayerCurrentGun(player).isDualGun)
        player->isRightGun = !player->isRightGun;
    else
        player->isRightGun = true;

    // Set the recoil
    if (player->isRightGun)
    {
        player->rightGunXRecoil = GunMaxRecoil;
        player->rightGunYRecoil = GunMaxRecoil;
    }
    else
    {
        player->leftGunXRecoil = GunMaxRecoil;
        player->leftGunYRecoil = GunMaxRecoil;
    }

    // Set muzzle flash and rumble
    if (player == &AllPlayers[CurrentCameraPlayer])
    {
        ShowMuzzle = 3;
        rumble(1);
    }
}

/**
 * @brief Stop gun reloading
 *
 * @param playerIndx Player index
 */
void StopReloading(int playerIndx)
{
    AllPlayers[playerIndx].isReloading = false;
}

/**
 * @brief Start gun reloading
 *
 * @param playerIndx Player index
 */
void startReloadGun(int playerIndx)
{
    if (playerIndx == 0)
    {
        DisableAim();
    }
    Player *player = &AllPlayers[playerIndx];
    if (player->currentGunInInventory == 1 || player->currentGunInInventory == 2)
    {
        if (!player->isReloading && getPlayerCurrentGun(player).MagazineCapacity != player->AllAmmoMagazine[player->currentGunInInventory - 1].AmmoCount && player->AllAmmoMagazine[player->currentGunInInventory - 1].TotalAmmoCount != 0)
        {
            player->GunReloadWaitCount = 0;
            player->isReloading = true;
        }
    }
}

/**
 * @brief Get the BMP of the gun in the hand of the viewed player
 *
 * @return const void* BMP data, NULL if the player has no gun in the hand
 */
static const void *getViewedGunBMP(void)
{
    Player *viewedPlayer = &AllPlayers[CurrentCameraPlayer];
    if (viewedPlayer->AllGunsInInventory[viewedPlayer->currentGunInInventory] == EMPTY)
        return NULL;

    int gunIndex = getPlayerCurrentGunIndex(viewedPlayer);
    if (gunIndex < GunCount)
        return getPlayerCurrentGun(viewedPlayer).texture; // Gun texture
    else if (gunIndex < GunCount + shopGrenadeCount)
        return AllGrenades[gunIndex - GunCount].texture; // Grenade texture
    else
        return allEquipments[gunIndex - GunCount - shopGrenadeCount].texture; // Equipment texture
}

/**
 * @brief Update weapons/grenades texture. The texture is copied to VRAM at the
 * start of the next vertical blank, so the screens don't blink.
 */
void UpdateGunTexture()
{
    const void *bmp = getViewedGunBMP();
    if (bmp == NULL)
        return;

    CS_TextureLoadDeferred(&TopScreenSpritesMaterials[1], &Palettes[4], bmp, 1);
}
