// SPDX-License-Identifier: MIT
//
// Copyright (c) 2008-2011, 2019, Antonio Niño Díaz
// Copyright (c) 2022, Fewnity - Grégory Machefer (Counter-Strike DS changes)
//
// AABB physics for Counter-Strike DS.
//
// This is the physics module of the modified Nitro Engine 0.7.0 used by
// Counter-Strike DS, moved into the game so that the official, unmodified
// Nitro Engine (0.15.x) can be used. Differences from Nitro Engine's own
// NEPhysics:
//
// - Each object can belong to up to CS_PHYSICS_MAX_GROUPS groups.
//   The updated object uses its first group (physicsgroup[0]) and collides with
//   every object that has that group in any of its slots.
// - iscollidingTrigger: true only during the first update of a collision.
// - NE_ColBounce bounces only vertically. Horizontally the object stops and
//   slides along walls.
// - Larger object limit (CS_PHYSICS_MAX).
//
// The enums NE_PhysicsTypes and NE_OnCollision from Nitro Engine are reused.

#ifndef CS_PHYSICS_H_
#define CS_PHYSICS_H_

#include <NEMain.h>

// 213 walls (Mirage) + 10 players + 20 grenades = 243 objects in the worst case
#define CS_PHYSICS_MAX 256
#define CS_PHYSICS_MAX_GROUPS 2
#define CS_MIN_BOUNCE_SPEED (floattof32(0.01))

typedef struct
{
    // Every fixed point variable in this struct is f32
    NE_Model *model;
    NE_PhysicsTypes type;
    bool enabled;

    // Speed of model. The coordinates are taken from the NE_Model struct
    int xspeed, yspeed, zspeed;

    // For Axis-Aligned Bounding Boxes
    int xsize, ysize, zsize;

    int gravity, friction;

    // Percentage of energy in an object after bouncing
    int keptpercent;

    // What to do when a collision is detected
    NE_OnCollision oncollision;

    // true if a collision was detected during the last update of the model
    bool iscolliding;
    bool lastIscolliding;
    // true only during the first update of a new collision
    bool iscollidingTrigger;

    // Objects only collide with other objects that have physicsgroup[0] of the
    // updated object in one of their groups
    int physicsgroupCount;
    int physicsgroup[CS_PHYSICS_MAX_GROUPS];
} CS_Physics;

CS_Physics *CS_PhysicsCreate(NE_PhysicsTypes type);
void CS_PhysicsDelete(CS_Physics *pointer);
void CS_PhysicsDeleteAll(void);
int CS_PhysicsSystemReset(int number_of_objects);
void CS_PhysicsSystemEnd(void);

void CS_PhysicsSetSpeedI(CS_Physics *pointer, int x, int y, int z);
#define CS_PhysicsSetSpeed(p, x, y, z) \
    CS_PhysicsSetSpeedI(p, floattof32(x), floattof32(y), floattof32(z))

void CS_PhysicsSetSizeI(CS_Physics *pointer, int x, int y, int z);
#define CS_PhysicsSetSize(p, x, y, z) \
    CS_PhysicsSetSizeI(p, floattof32(x), floattof32(y), floattof32(z))

void CS_PhysicsSetGravityI(CS_Physics *pointer, int gravity);
#define CS_PhysicsSetGravity(p, g) CS_PhysicsSetGravityI(p, floattof32(g))

void CS_PhysicsSetFrictionI(CS_Physics *pointer, int friction);
#define CS_PhysicsSetFriction(p, f) CS_PhysicsSetFrictionI(p, floattof32(f))

void CS_PhysicsSetBounceEnergy(CS_Physics *pointer, int percent);
void CS_PhysicsEnable(CS_Physics *pointer, bool value);
void CS_PhysicsSetModel(CS_Physics *physics, NE_Model *modelpointer);

// Sets the group in slot "index" and makes sure physicsgroupCount covers it
void CS_PhysicsSetGroup(CS_Physics *physics, int group, int index);

void CS_PhysicsOnCollision(CS_Physics *physics, NE_OnCollision action);
bool CS_PhysicsIsColliding(const CS_Physics *pointer);

void CS_PhysicsUpdateAll(void);
void CS_PhysicsUpdate(CS_Physics *pointer);
bool CS_PhysicsCheckCollision(const CS_Physics *pointer1,
                              const CS_Physics *pointer2);

#endif // CS_PHYSICS_H_
