// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021-2022, Fewnity - Grégory Machefer
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

#include "map.h"
#include "collisions.h"
#include "party.h"
#include "data/ai_data.h"
#include "camera.h"
#include "draw3d.h"

// Maps data
Map allMaps[MAP_COUNT];

/**
 * @brief Unload a map
 *
 * @param mapToUnload Map index to unload
 */
void UnLoadMap(int mapToUnload)
{
    // Destroy all collisions (models and physics)
    for (int i = 0; i < allMaps[mapToUnload].CollisionsCount; i++)
    {
        NE_PhysicsDelete(allMaps[mapToUnload].AllWallsCollisions[i].WallPhysics);
        NE_ModelDelete(allMaps[mapToUnload].AllWallsCollisions[i].WallModel);
    }

    // Destroy all map models
    for (int i = 0; i < allMaps[mapToUnload].occlusionZoneCount; i++)
    {
        NE_ModelDelete(allMaps[mapToUnload].models[i].Model);
    }

    // Free all used memory used by the map
    if (allMaps[mapToUnload].AllWallsCollisions != NULL)
        free(allMaps[mapToUnload].AllWallsCollisions);

    if (allMaps[mapToUnload].models != NULL)
        free(allMaps[mapToUnload].models);

    if (allMaps[mapToUnload].AllStairs != NULL)
        free(allMaps[mapToUnload].AllStairs);

    for (int i = 0; i < allMaps[mapToUnload].BombsTriggersCollisionsCount; i++)
    {
        free(allMaps[mapToUnload].AllBombsTriggersCollisions[i].nearWaypoints);
    }

    if (allMaps[mapToUnload].AllBombsTriggersCollisions != NULL)
        free(allMaps[mapToUnload].AllBombsTriggersCollisions);

    if (allMaps[mapToUnload].AllZones != NULL)
        free(allMaps[mapToUnload].AllZones);

    if (allMaps[mapToUnload].AllShadowCollisionBox != NULL)
        free(allMaps[mapToUnload].AllShadowCollisionBox);

    LastStairs = 0;

    // Destroy grenades
    for (int i = 0; i < GrenadeCount; i++)
    {
        DeleteGrenade(i);
    }
}

/**
 * @brief Load a map
 *
 * @param mapToLoad Map index to load
 */
void LoadMap(int mapToLoad)
{
    CreateWaypoints(mapToLoad);
    createLengthMatrices(mapToLoad);

    LoadAllCollisions(mapToLoad);
    CalculateAllTriggerColBoxs(mapToLoad);
    LoadAllStairs(mapToLoad);

    LoadAllBombZones(mapToLoad);
    loadMapModels(mapToLoad);
    setCameraMapPosition();
}

/**
 * @brief Load map images for the map list
 *
 */
void LoadMapTextures()
{
    allMaps[DUST2].image = (void *)tex_map_image_dust2_bin;
    allMaps[TUTORIAL].image = (void *)tex_map_image_tutorial_bin;
    allMaps[DUST2_2x2].image = (void *)tex_map_image_dust2_2x2_bin;
    allMaps[AIM_MAP].image = (void *)tex_map_image_aim_bin;
    allMaps[B2000].image = (void *)tex_map_image_2000_bin;
    allMaps[DUST2].MapImage = (void *)tex_Dust2_bin;
    allMaps[AIM_MAP].MapImage = (void *)tex_General_Map_bin;
}

/**
 * @brief Set maps names
 *
 */
void SetMapNames()
{
    allMaps[DUST2].name = "Dust II";
    allMaps[TUTORIAL].name = "Training (unfinished)";
    allMaps[DUST2_2x2].name = "Dust II_2x2";
    allMaps[AIM_MAP].name = "Aim_Map";
    allMaps[B2000].name = "$2000$";
}

/**
 * @brief Set maps party mode (-1 = no party mode forced)
 *
 */
void SetMapPartyMode()
{
    allMaps[DUST2].forcePartyMode = -1;
    allMaps[TUTORIAL].forcePartyMode = 2;
    allMaps[DUST2_2x2].forcePartyMode = -1;
    allMaps[AIM_MAP].forcePartyMode = -1;
    allMaps[B2000].forcePartyMode = -1;
}

/**
 * @brief Set the map camera position (used when the player is not in game)
 *
 */
void SetMapCameraPosition()
{
    // DUST2
    allMaps[DUST2].cameraPosition.x = -3;
    allMaps[DUST2].cameraPosition.y = 8;
    allMaps[DUST2].cameraPosition.z = -14;

    allMaps[DUST2].cameraLookPosition.x = -12;
    allMaps[DUST2].cameraLookPosition.y = 3.2;
    allMaps[DUST2].cameraLookPosition.z = -16;

    // TUTORIAL
    allMaps[TUTORIAL].cameraPosition.x = -12;
    allMaps[TUTORIAL].cameraPosition.y = 8;
    allMaps[TUTORIAL].cameraPosition.z = -5;

    allMaps[TUTORIAL].cameraLookPosition.x = -3;
    allMaps[TUTORIAL].cameraLookPosition.y = 3.2;
    allMaps[TUTORIAL].cameraLookPosition.z = -0;

    // DUST2_2x2
    allMaps[DUST2_2x2].cameraPosition.x = 53;
    allMaps[DUST2_2x2].cameraPosition.y = 7;
    allMaps[DUST2_2x2].cameraPosition.z = -14;

    allMaps[DUST2_2x2].cameraLookPosition.x = -3;
    allMaps[DUST2_2x2].cameraLookPosition.y = -15.2;
    allMaps[DUST2_2x2].cameraLookPosition.z = -50;

    // Aim Map
    allMaps[AIM_MAP].cameraPosition.x = -12;
    allMaps[AIM_MAP].cameraPosition.y = 8;
    allMaps[AIM_MAP].cameraPosition.z = -5;

    allMaps[AIM_MAP].cameraLookPosition.x = -3;
    allMaps[AIM_MAP].cameraLookPosition.y = 3.2;
    allMaps[AIM_MAP].cameraLookPosition.z = -0;

    // $2000$
    allMaps[B2000].cameraPosition.x = -19;
    allMaps[B2000].cameraPosition.y = 5;
    allMaps[B2000].cameraPosition.z = -21;

    allMaps[B2000].cameraLookPosition.x = -3;
    allMaps[B2000].cameraLookPosition.y = 3.2;
    allMaps[B2000].cameraLookPosition.z = -0;
}

/**
 * @brief Set maps spawns positions
 *
 */
void AddAllSpawnPoints()
{
    // DUST2
    AddSpawnPoint(DUST2, TERRORISTS, 0, -3, 6.43, 65);
    AddSpawnPoint(DUST2, TERRORISTS, 1, -6, 6.43, 65);
    AddSpawnPoint(DUST2, TERRORISTS, 2, -9, 6.43, 65);
    AddSpawnPoint(DUST2, TERRORISTS, 3, -3, 6.43, 62.5);
    AddSpawnPoint(DUST2, TERRORISTS, 4, -6, 6.43, 62.5);
    AddSpawnPoint(DUST2, TERRORISTS, 5, -9, 6.43, 62.5);

    AddSpawnPoint(DUST2, COUNTERTERRORISTS, 0, 16, 0, -22);
    AddSpawnPoint(DUST2, COUNTERTERRORISTS, 1, 19.5, 0, -22);
    AddSpawnPoint(DUST2, COUNTERTERRORISTS, 2, 23, 0, -22);
    AddSpawnPoint(DUST2, COUNTERTERRORISTS, 3, 16, 0, -19.5);
    AddSpawnPoint(DUST2, COUNTERTERRORISTS, 4, 19.5, 0, -22);
    AddSpawnPoint(DUST2, COUNTERTERRORISTS, 5, 23, 0, -22);

    SetAngles(DUST2, 0, -256);

    // TUTORIAL
    AddSpawnPoint(TUTORIAL, 0, 0, 0, 0, 0);

    AddSpawnPoint(TUTORIAL, 1, 0, 0, 0, 0);

    SetAngles(TUTORIAL, 0, 0);

    // DUST2_2x2
    AddSpawnPoint(DUST2_2x2, TERRORISTS, 0, -3, 6.43, 65);
    AddSpawnPoint(DUST2_2x2, TERRORISTS, 1, -6, 6.43, 65);
    AddSpawnPoint(DUST2_2x2, TERRORISTS, 2, -9, 6.43, 65);
    AddSpawnPoint(DUST2_2x2, TERRORISTS, 3, -3, 6.43, 62.5);
    AddSpawnPoint(DUST2_2x2, TERRORISTS, 4, -6, 6.43, 62.5);
    AddSpawnPoint(DUST2_2x2, TERRORISTS, 5, -9, 6.43, 62.5);

    AddSpawnPoint(DUST2_2x2, COUNTERTERRORISTS, 0, 16, 0, -22);
    AddSpawnPoint(DUST2_2x2, COUNTERTERRORISTS, 1, 19.5, 0, -22);
    AddSpawnPoint(DUST2_2x2, COUNTERTERRORISTS, 2, 23, 0, -22);
    AddSpawnPoint(DUST2_2x2, COUNTERTERRORISTS, 3, 16, 0, -19.5);
    AddSpawnPoint(DUST2_2x2, COUNTERTERRORISTS, 4, 19.5, 0, -22);
    AddSpawnPoint(DUST2_2x2, COUNTERTERRORISTS, 5, 23, 0, -22);

    SetAngles(DUST2_2x2, 0, -256);

    // Aim_Map
    AddSpawnPoint(AIM_MAP, TERRORISTS, 0, 4, 0, 16.2);
    AddSpawnPoint(AIM_MAP, TERRORISTS, 1, -2.5, 0, 16.2);
    AddSpawnPoint(AIM_MAP, TERRORISTS, 2, 12, 0, 16.2);
    AddSpawnPoint(AIM_MAP, TERRORISTS, 3, 4, 0, 18.5);
    AddSpawnPoint(AIM_MAP, TERRORISTS, 4, -2.5, 0, 18.5);
    AddSpawnPoint(AIM_MAP, TERRORISTS, 5, 12, 0, 18.5);

    AddSpawnPoint(AIM_MAP, COUNTERTERRORISTS, 0, -3.7, 0, -15.44);
    AddSpawnPoint(AIM_MAP, COUNTERTERRORISTS, 1, -11.82, 0, -15.44);
    AddSpawnPoint(AIM_MAP, COUNTERTERRORISTS, 2, 2.71, 0, -15.44);
    AddSpawnPoint(AIM_MAP, COUNTERTERRORISTS, 3, -3.7, 0, -17.74);
    AddSpawnPoint(AIM_MAP, COUNTERTERRORISTS, 4, -11.82, 0, -17.74);
    AddSpawnPoint(AIM_MAP, COUNTERTERRORISTS, 5, 2.71, 0, -17.74);

    SetAngles(AIM_MAP, 0, -256);

    // $2000$
    AddSpawnPoint(B2000, TERRORISTS, 0, 0, 0, -14.3);
    AddSpawnPoint(B2000, TERRORISTS, 1, -7, 0, -14.3);
    AddSpawnPoint(B2000, TERRORISTS, 2, 7, 0, -14.3);
    AddSpawnPoint(B2000, TERRORISTS, 3, 0, 0, -17.3);
    AddSpawnPoint(B2000, TERRORISTS, 4, -7, 0, -17.3);
    AddSpawnPoint(B2000, TERRORISTS, 5, 7, 0, -17.3);

    AddSpawnPoint(B2000, COUNTERTERRORISTS, 0, 0, 0, 14.3);
    AddSpawnPoint(B2000, COUNTERTERRORISTS, 1, -7, 0, 14.3);
    AddSpawnPoint(B2000, COUNTERTERRORISTS, 2, 7, 0, 14.3);
    AddSpawnPoint(B2000, COUNTERTERRORISTS, 3, 0, 0, 17.3);
    AddSpawnPoint(B2000, COUNTERTERRORISTS, 4, -7, 0, 17.3);
    AddSpawnPoint(B2000, COUNTERTERRORISTS, 5, 7, 0, 17.3);

    SetAngles(B2000, -256, 0);
}

/**
 * @brief Load map models
 *
 * @param mapToLoad
 */
void loadMapModels(int mapToLoad)
{
    Map *map = &allMaps[mapToLoad];

    Vector3 mapPos;
    Vector3Int Scale;
    int mapPartCount = 0;

    // Set map position, scale and parts count,
    if (mapToLoad == DUST2)
    {
        mapPartCount = 14;
        mapPos.x = 0;
        mapPos.y = 1.5 + 0.8;
        mapPos.z = 0;
        Scale.x = Scale.y = Scale.z = 4096;
    }
    else if (mapToLoad == TUTORIAL)
    {
        mapPartCount = 2;
        mapPos.x = 0;
        mapPos.y = -1.5 + 0.5;
        mapPos.z = 0;
        Scale.x = Scale.y = Scale.z = 2048;
    }
    else if (mapToLoad == DUST2_2x2)
    {
        mapPartCount = 8;
        mapPos.x = 0;
        mapPos.y = 1.5 + 0.8;
        mapPos.z = 0;
        Scale.x = Scale.y = Scale.z = 4096;
    }
    else if (mapToLoad == AIM_MAP)
    {
        mapPartCount = 6;
        mapPos.x = 0;
        mapPos.y = -1.5 + 0.5;
        mapPos.z = 0;
        Scale.x = Scale.y = Scale.z = 2048;
    }
    else if (mapToLoad == B2000)
    {
        mapPartCount = 4;
        mapPos.x = 0;
        mapPos.y = -1.5 + 0.5;
        mapPos.z = 0;
        Scale.x = Scale.y = Scale.z = 2048;
    }

    // Set for each map parts, position, rotation and scale
    for (int i = 0; i < mapPartCount; i++)
    {
        map->models[i].Model = NE_ModelCreate(NE_Static);
        NE_ModelSetMaterial(map->models[i].Model, GroundMaterial);
        NE_ModelScaleI(map->models[i].Model, Scale.x, Scale.y, Scale.z);
        NE_ModelSetCoord(map->models[i].Model, mapPos.x, mapPos.y, mapPos.z);
        map->models[i].Model->rx = 128;
        map->models[i].Model->ry = 256;
    }
    // if(CurrentTexture == 1)
    // {
    //     for (int i = 0; i < mapPartCount; i++)
    //     {
    //         map->models[i].Model = NE_ModelCreate(NE_Static);
    //         NE_ModelSetMaterial(map->models[i].Model, GroundMaterial);
    //         NE_ModelScaleI(map->models[i].Model, Scale.x, Scale.y, Scale.z);
    //         NE_ModelSetCoord(map->models[i].Model, mapPos.x, mapPos.y, mapPos.z);
    //         map->models[i].Model->rx = 128;
    //         map->models[i].Model->ry = 256;
    //     }
    // }
    // else if(CurrentTexture == 2)
    // {
    //     for (int i = 0; i < mapPartCount; i++)
    //     {
    //         map->models[i].Model = NE_ModelCreate(NE_Static);
    //         NE_ModelSetMaterial(map->models[i].Model, GroundMaterial1);
    //         NE_ModelScaleI(map->models[i].Model, Scale.x, Scale.y, Scale.z);
    //         NE_ModelSetCoord(map->models[i].Model, mapPos.x, mapPos.y, mapPos.z);
    //         map->models[i].Model->rx = 128;
    //         map->models[i].Model->ry = 256;
    //     }
    // }

    // Set for each map parts, the model and the position
    if (mapToLoad == DUST2)
    {
        NE_ModelLoadStaticMesh(map->models[0].Model, (u32 *)m_DustPart0Solid_bin);
        map->models[0].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[1].Model, (u32 *)m_DustPart1Solid_bin);
        map->models[1].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[2].Model, (u32 *)m_DustPart2Solid_bin);
        map->models[2].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[3].Model, (u32 *)m_DustPart3Solid_bin);
        map->models[3].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[4].Model, (u32 *)m_DustPart4Solid_bin);
        map->models[4].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[5].Model, (u32 *)m_DustPart5Solid_bin);
        map->models[5].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[6].Model, (u32 *)m_DustPart6Solid_bin);
        map->models[6].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[7].Model, (u32 *)m_DustPart0Shadows_bin);
        map->models[7].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[8].Model, (u32 *)m_DustPart1Shadows_bin);
        map->models[8].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[9].Model, (u32 *)m_DustPart2Shadows_bin);
        map->models[9].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[10].Model, (u32 *)m_DustPart3Shadows_bin);
        map->models[10].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[11].Model, (u32 *)m_DustPart4Shadows_bin);
        map->models[11].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[12].Model, (u32 *)m_DustPart5Shadows_bin);
        map->models[12].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[13].Model, (u32 *)m_DustPart6Shadows_bin);
        map->models[13].shadowed = true;
    }
    else if (mapToLoad == TUTORIAL)
    {
        NE_ModelLoadStaticMesh(map->models[0].Model, (u32 *)m_tutorialMapUnShadowed_3ds_bin);
        map->models[0].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[1].Model, (u32 *)m_tutorialMapShadowed_3ds_bin);
        map->models[1].shadowed = true;
    }
    else if (mapToLoad == DUST2_2x2)
    {
        NE_ModelLoadStaticMesh(map->models[0].Model, (u32 *)m_Dust2_2x2Part3Solid_bin);
        map->models[0].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[1].Model, (u32 *)m_Dust2_2x2Part3Shadows_bin);
        map->models[1].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[2].Model, (u32 *)m_Dust2_2x2Part4Solid_bin);
        map->models[2].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[3].Model, (u32 *)m_Dust2_2x2Part4Shadows_bin);
        map->models[3].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[4].Model, (u32 *)m_DustPart5Solid_bin);
        map->models[4].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[5].Model, (u32 *)m_DustPart5Shadows_bin);
        map->models[5].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[6].Model, (u32 *)m_DustPart6Solid_bin);
        map->models[6].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[7].Model, (u32 *)m_DustPart6Shadows_bin);
        map->models[7].shadowed = true;
    }
    else if (mapToLoad == AIM_MAP)
    {
        NE_ModelLoadStaticMesh(map->models[0].Model, (u32 *)m_Aim_MapPart0Solid_bin);
        map->models[0].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[1].Model, (u32 *)m_Aim_MapPart0Shadows_bin);
        map->models[1].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[2].Model, (u32 *)m_Aim_MapPart1Solid_bin);
        map->models[2].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[3].Model, (u32 *)m_Aim_MapPart1Shadows_bin);
        map->models[3].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[4].Model, (u32 *)m_Aim_MapPart2Solid_bin);
        map->models[4].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[5].Model, (u32 *)m_Aim_MapPart2Shadows_bin);
        map->models[5].shadowed = true;
    }
    else if (mapToLoad == B2000)
    {
        NE_ModelLoadStaticMesh(map->models[0].Model, (u32 *)m_2000Part0Solid_bin);
        map->models[0].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[1].Model, (u32 *)m_2000Part0Shadows_bin);
        map->models[1].shadowed = true;
        NE_ModelLoadStaticMesh(map->models[2].Model, (u32 *)m_2000Part1Solid_bin);
        map->models[2].shadowed = false;
        NE_ModelLoadStaticMesh(map->models[3].Model, (u32 *)m_2000Part1Shadows_bin);
        map->models[3].shadowed = true;
    }
}

Wall *getMapWalls()
{
    return allMaps[currentMap].AllWallsCollisions;
}

int getMapWallsCount()
{
    return allMaps[currentMap].CollisionsCount;
}

/**
 * @brief Set map name
 *
 * @param mapIndex Map index
 * @param name Name
 */
void SetName(int mapIndex, const char *name)
{
    allMaps[mapIndex].name = name;
}

/**
 * @brief Set the spawn angle
 *
 * @param mapIndex Map index
 * @param terroristsAngle Spawn angle for terrorists
 * @param CounterAngle Spawn angle for counter terrorists
 */
void SetAngles(int mapIndex, float terroristsAngle, float CounterAngle)
{
    allMaps[mapIndex].startPlayerAngleTerrorists = terroristsAngle;
    allMaps[mapIndex].startPlayerAngleCounterTerrorists = CounterAngle;
}

/**
 * @brief Add a spawn point to the map
 *
 * @param mapIndex Map index
 * @param team Spawn point team
 * @param index Spawn point index
 * @param x X position
 * @param y Y position
 * @param z Z position
 */
void AddSpawnPoint(int mapIndex, int team, int index, float x, float y, float z)
{
    Map *map = &allMaps[mapIndex];
    if (team == TERRORISTS)
    {
        map->allTerroristsSpawns[index].x = x;
        map->allTerroristsSpawns[index].y = y;
        map->allTerroristsSpawns[index].z = z;
    }
    else
    {
        map->allCounterTerroristsSpawns[index].x = x;
        map->allCounterTerroristsSpawns[index].y = y;
        map->allCounterTerroristsSpawns[index].z = z;
    }
}
