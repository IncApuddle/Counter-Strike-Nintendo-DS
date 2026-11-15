// SPDX-License-Identifier: MIT
//
// Copyright (c) 2008-2011, 2019, Antonio Niño Díaz
// Copyright (c) 2022, Fewnity - Grégory Machefer (Counter-Strike DS changes)
//
// AABB physics for Counter-Strike DS. See cs_physics.h for the differences
// with the physics module of Nitro Engine.

#include <stdlib.h>

#include "cs_physics.h"

static CS_Physics **cs_physics_pointers = NULL;
static int cs_physics_max = 0;

CS_Physics *CS_PhysicsCreate(NE_PhysicsTypes type)
{
    if (cs_physics_pointers == NULL)
        return NULL;

    if (type != NE_BoundingBox)
    {
        NE_DebugPrint("Only bounding boxes are supported");
        return NULL;
    }

    for (int i = 0; i < cs_physics_max; i++)
    {
        if (cs_physics_pointers[i] != NULL)
            continue;

        CS_Physics *temp = calloc(1, sizeof(CS_Physics));
        if (temp == NULL)
        {
            NE_DebugPrint("Not enough memory");
            return NULL;
        }

        temp->type = type;
        temp->keptpercent = 50;
        temp->enabled = true;
        temp->physicsgroupCount = 1;
        temp->oncollision = NE_ColNothing;

        cs_physics_pointers[i] = temp;
        return temp;
    }

    NE_DebugPrint("No free physics slots");
    return NULL;
}

void CS_PhysicsDelete(CS_Physics *pointer)
{
    if (cs_physics_pointers == NULL || pointer == NULL)
        return;

    for (int i = 0; i < cs_physics_max; i++)
    {
        if (cs_physics_pointers[i] == pointer)
        {
            cs_physics_pointers[i] = NULL;
            free(pointer);
            return;
        }
    }

    NE_DebugPrint("Object not found");
}

void CS_PhysicsDeleteAll(void)
{
    if (cs_physics_pointers == NULL)
        return;

    for (int i = 0; i < cs_physics_max; i++)
    {
        free(cs_physics_pointers[i]);
        cs_physics_pointers[i] = NULL;
    }
}

int CS_PhysicsSystemReset(int number_of_objects)
{
    CS_PhysicsSystemEnd();

    cs_physics_max = (number_of_objects < 1) ? CS_PHYSICS_MAX : number_of_objects;

    cs_physics_pointers = calloc(cs_physics_max, sizeof(CS_Physics *));
    if (cs_physics_pointers == NULL)
    {
        NE_DebugPrint("Not enough memory");
        cs_physics_max = 0;
        return -1;
    }

    return 0;
}

void CS_PhysicsSystemEnd(void)
{
    if (cs_physics_pointers == NULL)
        return;

    CS_PhysicsDeleteAll();
    free(cs_physics_pointers);
    cs_physics_pointers = NULL;
    cs_physics_max = 0;
}

void CS_PhysicsSetSpeedI(CS_Physics *pointer, int x, int y, int z)
{
    NE_AssertPointer(pointer, "NULL pointer");
    pointer->xspeed = x;
    pointer->yspeed = y;
    pointer->zspeed = z;
}

void CS_PhysicsSetSizeI(CS_Physics *pointer, int x, int y, int z)
{
    NE_AssertPointer(pointer, "NULL pointer");
    NE_Assert(x >= 0 && y >= 0 && z >= 0, "Size must be positive");
    pointer->xsize = x;
    pointer->ysize = y;
    pointer->zsize = z;
}

void CS_PhysicsSetGravityI(CS_Physics *pointer, int gravity)
{
    NE_AssertPointer(pointer, "NULL pointer");
    pointer->gravity = gravity;
}

void CS_PhysicsSetFrictionI(CS_Physics *pointer, int friction)
{
    NE_AssertPointer(pointer, "NULL pointer");
    NE_Assert(friction >= 0, "Friction must be positive");
    pointer->friction = friction;
}

void CS_PhysicsSetBounceEnergy(CS_Physics *pointer, int percent)
{
    NE_AssertPointer(pointer, "NULL pointer");
    NE_Assert(percent >= 0, "Percentage must be positive");
    pointer->keptpercent = percent;
}

void CS_PhysicsEnable(CS_Physics *pointer, bool value)
{
    NE_AssertPointer(pointer, "NULL pointer");
    pointer->enabled = value;
}

void CS_PhysicsSetModel(CS_Physics *physics, NE_Model *modelpointer)
{
    NE_AssertPointer(physics, "NULL physics pointer");
    NE_AssertPointer(modelpointer, "NULL model pointer");
    physics->model = modelpointer;
}

void CS_PhysicsSetGroup(CS_Physics *physics, int group, int index)
{
    NE_AssertPointer(physics, "NULL pointer");
    if (index < 0 || index >= CS_PHYSICS_MAX_GROUPS)
    {
        NE_DebugPrint("Invalid group index");
        return;
    }

    physics->physicsgroup[index] = group;
    if (physics->physicsgroupCount < index + 1)
        physics->physicsgroupCount = index + 1;
}

void CS_PhysicsOnCollision(CS_Physics *physics, NE_OnCollision action)
{
    NE_AssertPointer(physics, "NULL pointer");
    physics->oncollision = action;
}

bool CS_PhysicsIsColliding(const CS_Physics *pointer)
{
    NE_AssertPointer(pointer, "NULL pointer");
    return pointer->iscolliding;
}

void CS_PhysicsUpdateAll(void)
{
    if (cs_physics_pointers == NULL)
        return;

    for (int i = 0; i < cs_physics_max; i++)
    {
        if (cs_physics_pointers[i] != NULL)
            CS_PhysicsUpdate(cs_physics_pointers[i]);
    }
}

static bool cs_physics_share_group(const CS_Physics *pointer,
                                   const CS_Physics *other)
{
    int count = other->physicsgroupCount;
    if (count > CS_PHYSICS_MAX_GROUPS)
        count = CS_PHYSICS_MAX_GROUPS;

    for (int j = 0; j < count; j++)
    {
        if (other->physicsgroup[j] == pointer->physicsgroup[0])
            return true;
    }

    return false;
}

ARM_CODE void CS_PhysicsUpdate(CS_Physics *pointer)
{
    if (cs_physics_pointers == NULL || pointer == NULL)
        return;

    NE_AssertPointer(pointer->model, "NULL model pointer");

    if (pointer->enabled == false)
        return;

    pointer->iscolliding = false;

    // Apply gravity
    pointer->yspeed -= pointer->gravity;

    // Move the object
    NE_Model *model = pointer->model;
    int bposx = model->x;
    int bposy = model->y;
    int bposz = model->z;
    int posx = model->x = model->x + pointer->xspeed;
    int posy = model->y = model->y + pointer->yspeed;
    int posz = model->z = model->z + pointer->zspeed;

    bool xenabled = (bposx != posx);
    bool yenabled = (bposy != posy);
    bool zenabled = (bposz != posz);

    for (int i = 0; i < cs_physics_max; i++)
    {
        CS_Physics *otherpointer = cs_physics_pointers[i];

        if (otherpointer == NULL || otherpointer == pointer)
            continue;

        if (!cs_physics_share_group(pointer, otherpointer))
            continue;

        if (otherpointer->model == NULL)
            continue;

        int otherposx = otherpointer->model->x;
        int otherposy = otherpointer->model->y;
        int otherposz = otherpointer->model->z;

        int xSizeSum = (pointer->xsize + otherpointer->xsize) >> 1;
        int ySizeSum = (pointer->ysize + otherpointer->ysize) >> 1;
        int zSizeSum = (pointer->zsize + otherpointer->zsize) >> 1;

        if (abs(posx - otherposx) >= xSizeSum)
            continue;
        if (abs(posy - otherposy) >= ySizeSum)
            continue;
        if (abs(posz - otherposz) >= zSizeSum)
            continue;

        pointer->iscolliding = true;

        if (pointer->oncollision == NE_ColBounce)
        {
            // Used to reduce speed:
            int temp = divf32(inttof32(pointer->keptpercent), inttof32(100));

            if (yenabled && (abs(bposy - otherposy) >= ySizeSum))
            {
                yenabled = false;
                pointer->yspeed += pointer->gravity;

                if (posy > otherposy)
                    pointer->model->y = otherposy + ySizeSum;
                if (posy < otherposy)
                    pointer->model->y = otherposy - ySizeSum;

                if (pointer->gravity == 0)
                {
                    pointer->yspeed = -mulf32(temp, pointer->yspeed);
                }
                else
                {
                    if (abs(pointer->yspeed) > CS_MIN_BOUNCE_SPEED)
                        pointer->yspeed = -mulf32(temp, pointer->yspeed - pointer->gravity);
                    else
                        pointer->yspeed = 0;
                }
            }

            // Horizontal collisions only stop the object (it slides along the
            // wall). They are ignored if the vertical collision has just moved
            // the object out of the other box (standing on it or hitting it
            // with the head).
            //
            // The original code had a misplaced parenthesis:
            //     abs((pointer->model->y - otherposy) < ySizeSum)
            // which ignored the "head hit" case.
            bool vertical_overlap = abs(pointer->model->y - otherposy) < ySizeSum;

            if (xenabled && (abs(bposx - otherposx) >= xSizeSum) && vertical_overlap)
            {
                xenabled = false;

                if (posx > otherposx)
                    pointer->model->x = otherposx + xSizeSum;
                if (posx < otherposx)
                    pointer->model->x = otherposx - xSizeSum;

                pointer->xspeed = 0;
            }

            if (zenabled && (abs(bposz - otherposz) >= zSizeSum) && vertical_overlap)
            {
                zenabled = false;

                if (posz > otherposz)
                    pointer->model->z = otherposz + zSizeSum;
                if (posz < otherposz)
                    pointer->model->z = otherposz - zSizeSum;

                pointer->zspeed = 0;
            }
        }
        else if (pointer->oncollision == NE_ColStop)
        {
            if (yenabled && (abs(bposy - otherposy) >= ySizeSum))
            {
                yenabled = false;

                if (posy > otherposy)
                    pointer->model->y = otherposy + ySizeSum;
                if (posy < otherposy)
                    pointer->model->y = otherposy - ySizeSum;
            }
            if (xenabled && (abs(bposx - otherposx) >= xSizeSum))
            {
                xenabled = false;

                if (posx > otherposx)
                    pointer->model->x = otherposx + xSizeSum;
                if (posx < otherposx)
                    pointer->model->x = otherposx - xSizeSum;
            }
            if (zenabled && (abs(bposz - otherposz) >= zSizeSum))
            {
                zenabled = false;

                if (posz > otherposz)
                    pointer->model->z = otherposz + zSizeSum;
                if (posz < otherposz)
                    pointer->model->z = otherposz - zSizeSum;
            }
            pointer->xspeed = pointer->yspeed = pointer->zspeed = 0;
        }
    }

    pointer->iscollidingTrigger = (!pointer->lastIscolliding && pointer->iscolliding);
    pointer->lastIscolliding = pointer->iscolliding;

    // Apply friction
    if (pointer->friction != 0)
    {
        pointer->xspeed <<= 10;
        pointer->yspeed <<= 10;
        pointer->zspeed <<= 10;
        int mod = mulf32(pointer->xspeed, pointer->xspeed);
        mod += mulf32(pointer->yspeed, pointer->yspeed);
        mod += mulf32(pointer->zspeed, pointer->zspeed);
        mod = sqrtf32(mod);

        // If the speed is very small, stop the object
        if (mod < pointer->friction)
        {
            pointer->xspeed = pointer->yspeed = pointer->zspeed = 0;
        }
        else
        {
            int newmod = mod - pointer->friction;
            // mod   --  newmod    ->  newspeed = speed * newmod / mod
            // speed --  newspeed
            int number = divf32(newmod, mod);
            pointer->xspeed = mulf32(pointer->xspeed, number) >> 10;
            pointer->yspeed = mulf32(pointer->yspeed, number) >> 10;
            pointer->zspeed = mulf32(pointer->zspeed, number) >> 10;
        }
    }
}

bool CS_PhysicsCheckCollision(const CS_Physics *pointer1,
                              const CS_Physics *pointer2)
{
    NE_AssertPointer(pointer1, "NULL pointer 1");
    NE_AssertPointer(pointer2, "NULL pointer 2");

    const NE_Model *m1 = pointer1->model;
    const NE_Model *m2 = pointer2->model;

    return (abs(m1->x - m2->x) < (pointer1->xsize + pointer2->xsize) >> 1)
        && (abs(m1->y - m2->y) < (pointer1->ysize + pointer2->ysize) >> 1)
        && (abs(m1->z - m2->z) < (pointer1->zsize + pointer2->zsize) >> 1);
}
