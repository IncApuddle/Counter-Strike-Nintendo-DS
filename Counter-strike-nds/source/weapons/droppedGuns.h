// SPDX-License-Identifier: MIT
//
// Copyright (c) 2026, Counter-Strike DS contributors
//
// Guns dropped on the floor (drop button and death). They are drawn with the
// small icon of the gun (one quad, see data/dropped_guns.bin).

#ifndef DROPPED_GUNS_H_
#define DROPPED_GUNS_H_

#include <stdbool.h>

#define MAX_DROPPED_GUNS 10

// Frames before the player who dropped a gun can take it back (2 s)
#define DROPPED_GUN_OWNER_DELAY 120

typedef struct
{
    bool used;
    int netId;       // Id given by the server (online), unused offline
    int gunId;       // Index in AllGuns
    int ammoCount;   // Ammo in the magazine
    int totalAmmo;   // Ammo outside of the magazine
    float x, y, z;   // Position on the floor
    int zone;        // Occlusion zone of the gun (-1 if not found)
    int ownerIndex;  // Player who dropped the gun
    int ownerDelay;  // Frames before the owner can take the gun back
    int age;         // Order of the drops (the oldest gun is removed first)
} DroppedGun;

extern DroppedGun droppedGuns[MAX_DROPPED_GUNS];
extern bool SendDropGun;

void initDroppedGuns(void);
void clearDroppedGuns(void);
bool canDropGunsInCurrentMode(void);
int addDroppedGun(int gunId, float x, float y, float z, int ammoCount, int totalAmmo, int ownerIndex, int netId);
void removeDroppedGunByNetId(int netId);

bool canPlayerDropCurrentGun(int playerIndex);
void dropCurrentGun(int playerIndex);
void dropGunOnDeath(int playerIndex);

void updateDroppedGuns(void);
void drawDroppedGuns(void);

#endif // DROPPED_GUNS_H_
