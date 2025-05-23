// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021-2022, Fewnity - Grégory Machefer
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

//#include "main.h"
#include "collisions.h"
#include "raycast.h"
#include "network.h"
#include "ai.h"
#include "gun.h"
#include "player.h"
#include "sounds.h"
#include "movements.h"
#include "map.h"
#include "party.h"

// All spawned grenades are here
PhysicalGrenade *grenades[GrenadeCount];
// Map index to load (for loading collisions)
int mapToSet = 0;

/**
 * @brief Add all trigger zones for occlusion culling
 *
 */
void CalculateAllTriggerColBoxs(int mapToLoad)
{
    Map *map = &allMaps[mapToLoad];
    int currentZone = 0;

    if (mapToLoad == DUST2)
    {
        // All occlusion culling zones data (a zone will show one or multiple zones)
        map->zonesCount = 21;
        map->AllZones = malloc(map->zonesCount * sizeof(Zone));

        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 4;
        map->AllZones[currentZone].visibleMapPart[2] = 6;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].visibleMapPart[4] = 11;
        map->AllZones[currentZone].visibleMapPart[5] = 13;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 1;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].visibleMapPart[4] = 8;
        map->AllZones[currentZone].visibleMapPart[5] = 11;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 2;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 7;
        map->AllZones[currentZone].visibleMapPart[3] = 8;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 3;
        map->AllZones[currentZone].ZoneCount = 7;
        map->AllZones[currentZone].visibleMapPart[0] = 1;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].visibleMapPart[4] = 8;
        map->AllZones[currentZone].visibleMapPart[5] = 10;
        map->AllZones[currentZone].visibleMapPart[6] = 11;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 4;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 2;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].visibleMapPart[4] = 8;
        map->AllZones[currentZone].visibleMapPart[5] = 9;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 5;
        map->AllZones[currentZone].ZoneCount = 8;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 6;
        map->AllZones[currentZone].visibleMapPart[4] = 7;
        map->AllZones[currentZone].visibleMapPart[5] = 8;
        map->AllZones[currentZone].visibleMapPart[6] = 11;
        map->AllZones[currentZone].visibleMapPart[7] = 13;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 6;
        map->AllZones[currentZone].ZoneCount = 8;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 6;
        map->AllZones[currentZone].visibleMapPart[4] = 7;
        map->AllZones[currentZone].visibleMapPart[5] = 10;
        map->AllZones[currentZone].visibleMapPart[6] = 11;
        map->AllZones[currentZone].visibleMapPart[7] = 13;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 7;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 4;
        map->AllZones[currentZone].visibleMapPart[1] = 6;
        map->AllZones[currentZone].visibleMapPart[2] = 11;
        map->AllZones[currentZone].visibleMapPart[3] = 13;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 8;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 4;
        map->AllZones[currentZone].visibleMapPart[1] = 5;
        map->AllZones[currentZone].visibleMapPart[2] = 6;
        map->AllZones[currentZone].visibleMapPart[3] = 11;
        map->AllZones[currentZone].visibleMapPart[4] = 12;
        map->AllZones[currentZone].visibleMapPart[5] = 13;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 9;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 5;
        map->AllZones[currentZone].visibleMapPart[1] = 6;
        map->AllZones[currentZone].visibleMapPart[2] = 12;
        map->AllZones[currentZone].visibleMapPart[3] = 13;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 10;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 5;
        map->AllZones[currentZone].visibleMapPart[1] = 6;
        map->AllZones[currentZone].visibleMapPart[2] = 12;
        map->AllZones[currentZone].visibleMapPart[3] = 13;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 11;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 2;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 5;
        map->AllZones[currentZone].visibleMapPart[3] = 9;
        map->AllZones[currentZone].visibleMapPart[4] = 10;
        map->AllZones[currentZone].visibleMapPart[5] = 12;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 12;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 1;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 8;
        map->AllZones[currentZone].visibleMapPart[4] = 10;
        map->AllZones[currentZone].visibleMapPart[5] = 11;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 13;
        map->AllZones[currentZone].ZoneCount = 8;
        map->AllZones[currentZone].visibleMapPart[0] = 1;
        map->AllZones[currentZone].visibleMapPart[1] = 2;
        map->AllZones[currentZone].visibleMapPart[2] = 3;
        map->AllZones[currentZone].visibleMapPart[3] = 5;
        map->AllZones[currentZone].visibleMapPart[4] = 8;
        map->AllZones[currentZone].visibleMapPart[5] = 9;
        map->AllZones[currentZone].visibleMapPart[6] = 10;
        map->AllZones[currentZone].visibleMapPart[7] = 12;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 14;
        map->AllZones[currentZone].ZoneCount = 10;
        map->AllZones[currentZone].visibleMapPart[0] = 1;
        map->AllZones[currentZone].visibleMapPart[1] = 2;
        map->AllZones[currentZone].visibleMapPart[2] = 3;
        map->AllZones[currentZone].visibleMapPart[3] = 4;
        map->AllZones[currentZone].visibleMapPart[4] = 5;
        map->AllZones[currentZone].visibleMapPart[5] = 8;
        map->AllZones[currentZone].visibleMapPart[6] = 9;
        map->AllZones[currentZone].visibleMapPart[7] = 10;
        map->AllZones[currentZone].visibleMapPart[8] = 11;
        map->AllZones[currentZone].visibleMapPart[9] = 12;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 15;
        map->AllZones[currentZone].ZoneCount = 8;
        map->AllZones[currentZone].visibleMapPart[0] = 2;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 5;
        map->AllZones[currentZone].visibleMapPart[4] = 9;
        map->AllZones[currentZone].visibleMapPart[5] = 10;
        map->AllZones[currentZone].visibleMapPart[6] = 11;
        map->AllZones[currentZone].visibleMapPart[7] = 12;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 16;
        map->AllZones[currentZone].ZoneCount = 8;
        map->AllZones[currentZone].visibleMapPart[0] = 1;
        map->AllZones[currentZone].visibleMapPart[1] = 2;
        map->AllZones[currentZone].visibleMapPart[2] = 3;
        map->AllZones[currentZone].visibleMapPart[3] = 5;
        map->AllZones[currentZone].visibleMapPart[4] = 8;
        map->AllZones[currentZone].visibleMapPart[5] = 9;
        map->AllZones[currentZone].visibleMapPart[6] = 10;
        map->AllZones[currentZone].visibleMapPart[7] = 12;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 17;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 1;
        map->AllZones[currentZone].visibleMapPart[1] = 2;
        map->AllZones[currentZone].visibleMapPart[2] = 3;
        map->AllZones[currentZone].visibleMapPart[3] = 8;
        map->AllZones[currentZone].visibleMapPart[4] = 9;
        map->AllZones[currentZone].visibleMapPart[5] = 10;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 18;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 3;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].visibleMapPart[4] = 8;
        map->AllZones[currentZone].visibleMapPart[5] = 10;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 19;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 3;
        map->AllZones[currentZone].visibleMapPart[1] = 4;
        map->AllZones[currentZone].visibleMapPart[2] = 6;
        map->AllZones[currentZone].visibleMapPart[3] = 10;
        map->AllZones[currentZone].visibleMapPart[4] = 11;
        map->AllZones[currentZone].visibleMapPart[5] = 13;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 20;
        map->AllZones[currentZone].ZoneCount = 8;
        map->AllZones[currentZone].visibleMapPart[0] = 1;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 6;
        map->AllZones[currentZone].visibleMapPart[4] = 8;
        map->AllZones[currentZone].visibleMapPart[5] = 10;
        map->AllZones[currentZone].visibleMapPart[6] = 11;
        map->AllZones[currentZone].visibleMapPart[7] = 13;
        map->AllZones[currentZone].id = currentZone;

        // Add occlusion culling triggers
        CalculateTriggerColBox(-9.52357, 57.34258, 15.70023, 20.76961, 0);
        CalculateTriggerColBox(-32.8, 31.70565, 25.39098, 32.87455, 1);
        CalculateTriggerColBox(-30.03613, 9.913523, 7.458398, 9.875763, 2);
        CalculateTriggerColBox(-12.07624, 8.24, 20.75247, 13, 3);
        CalculateTriggerColBox(-38.87369, 3.970799, 9.822498, 15.72273, 4);
        CalculateTriggerColBox(-31.71968, 57.28525, 27.55163, 17.77374, 5);
        CalculateTriggerColBox(10.59276, 58.97986, 23.88577, 17.49506, 6);
        CalculateTriggerColBox(22.83467, 41.8413, 16.72676, 16.5031, 7);
        CalculateTriggerColBox(27.63166, 29.56198, 6.307827, 7.687183, 8);
        CalculateTriggerColBox(29.65833, 17.5136, 11.42418, 16.10892, 9);
        CalculateTriggerColBox(47.04734, 20.07397, 22.30289, 42.11788, 10);
        CalculateTriggerColBox(31.25437, -19.58616, 52.12261, 37.01623, 11);
        CalculateTriggerColBox(11.91049, 2.419476, 25.90689, 6.575387, 12);
        CalculateTriggerColBox(-13.12243, -19.91082, 15.07712, 12.67715, 13);
        CalculateTriggerColBox(-11.61234, -19.34598, 18.0973, 11.54748, 14);
        CalculateTriggerColBox(-7.77587, -16.08564, 25.74453, 30.01518, 15);
        CalculateTriggerColBox(-31.87811, -8.83736, 22.03724, 15.51862, 16);
        CalculateTriggerColBox(-31.87811, -28.16135, 22.03724, 23.0819, 17);
        CalculateTriggerColBox(-24.4092, 11.52498, 3.830673, 6.652855, 18);
        CalculateTriggerColBox(0.2512362, 31.8348, 16.78329, 36.75443, 19);
        CalculateTriggerColBox(3.534717, 9.606822, 9.155338, 8.084706, 20);

        map->shadowCollisionBoxCount = 26;
        map->AllShadowCollisionBox = malloc(map->shadowCollisionBoxCount * sizeof(CollisionBox));

        // Add shadow collision boxes
        CalculateShadowColBox(18.34385, 1.727197, -16.21586, 19.7, 3.42, 14.1, 0);
        CalculateShadowColBox(37.52384, 4.967202, -1.585868, 22.3, 8.46, 24.08, 1);
        CalculateShadowColBox(31.71384, 5.157205, 26.63413, 15.97, 3.98, 15.1, 2);
        CalculateShadowColBox(38.72385, 5.047204, 31.65415, 5.91, 3.62, 6.38, 3);
        CalculateShadowColBox(50.48385, 3.157205, 35.40415, 13, 10.9, 11.83, 4);
        CalculateShadowColBox(26.97384, 6.007195, 15.57414, 6.1, 6.05, 11.29, 5);
        CalculateShadowColBox(38.82384, 8.397195, -33.39587, 4.38, 4.07, 8.44, 6);
        CalculateShadowColBox(19.78384, 7.157205, -14.67586, 4.89, 7.2, 28.1, 7);
        CalculateShadowColBox(18.25384, 6.437203, 5.844155, 16.05, 7.2, 5.4, 8);
        CalculateShadowColBox(-2.746141, 4.257195, 10.21415, 10.61, 11, 49.66, 9);
        CalculateShadowColBox(-13.06615, 3.667199, -11.26586, 16.59, 8.3, 6.9, 10);
        CalculateShadowColBox(-19.88616, 6.287194, -20.15585, 2.69, 6.7, 15.94, 11);
        CalculateShadowColBox(-40.42616, 6.287194, -21.91586, 4.5, 6.7, 34.8, 12);
        CalculateShadowColBox(-24.16615, 4.447198, 3.014138, 22.3, 8.53, 19.23, 13);
        CalculateShadowColBox(-25.63616, 4.447198, 17.13413, 19.7, 8.53, 10.9, 14);
        CalculateShadowColBox(-38.92616, 4.447198, 1.164132, 9.6, 8.53, 16.68, 15);
        CalculateShadowColBox(-39.73616, 7.347207, 36.37415, 9.6, 10.4, 25.84, 16);
        CalculateShadowColBox(-45.63615, 7.347207, 60.09415, 9.6, 9.3, 14.4, 17);
        CalculateShadowColBox(-28.22615, 3.937203, 43.60413, 9.6, 4.81, 4.37, 18);
        CalculateShadowColBox(-19.16615, 8.457208, 66.10413, 57.3, 11.4, 2.86, 19);
        CalculateShadowColBox(17.28386, 5.877206, 63.06413, 14.3, 6.7, 4.37, 20);
        CalculateShadowColBox(29.67384, 5.877206, 49.91413, 14.3, 6.7, 4.37, 21);
        CalculateShadowColBox(12.53386, 5.877206, 40.81413, 14.3, 6.7, 21, 22);
        CalculateShadowColBox(-3.626146, 5.877206, 40.73414, 12.42, 6.7, 31.4, 23);
        CalculateShadowColBox(-9.356157, 8.267205, 57.86414, 4.4, 5.6, 17.13, 24);
        CalculateShadowColBox(39.72386, 6.616204, -19.41586, 1.99, 2.22, 3.34, 25);

        map->occlusionZoneCount = 14;
        map->AllOcclusionZone = malloc(map->occlusionZoneCount * sizeof(OcclusionZone));

        // Add clipping zones
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[0].angles[0].x = -11.1;
        map->AllOcclusionZone[0].angles[0].y = 83.1;

        map->AllOcclusionZone[0].angles[1].x = -46.5;
        map->AllOcclusionZone[0].angles[1].y = 83.1;

        map->AllOcclusionZone[0].angles[2].x = -46.5;
        map->AllOcclusionZone[0].angles[2].y = 20.7;

        map->AllOcclusionZone[0].angles[3].x = -11.1;
        map->AllOcclusionZone[0].angles[3].y = 20.7;
        ////
        map->AllOcclusionZone[7].angles[0].x = -11.1;
        map->AllOcclusionZone[7].angles[0].y = 83.1;

        map->AllOcclusionZone[7].angles[1].x = -46.5;
        map->AllOcclusionZone[7].angles[1].y = 83.1;

        map->AllOcclusionZone[7].angles[2].x = -46.5;
        map->AllOcclusionZone[7].angles[2].y = 20.7;

        map->AllOcclusionZone[7].angles[3].x = -11.1;
        map->AllOcclusionZone[7].angles[3].y = 20.7;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[1].angles[0].x = -6.8;
        map->AllOcclusionZone[1].angles[0].y = 25.4;

        map->AllOcclusionZone[1].angles[1].x = -45.8;
        map->AllOcclusionZone[1].angles[1].y = 25.4;

        map->AllOcclusionZone[1].angles[2].x = -45.8;
        map->AllOcclusionZone[1].angles[2].y = -5.39;

        map->AllOcclusionZone[1].angles[3].x = -6.8;
        map->AllOcclusionZone[1].angles[3].y = -5.39;
        ////
        map->AllOcclusionZone[8].angles[0].x = -6.8;
        map->AllOcclusionZone[8].angles[0].y = 25.4;

        map->AllOcclusionZone[8].angles[1].x = -45.8;
        map->AllOcclusionZone[8].angles[1].y = 25.4;

        map->AllOcclusionZone[8].angles[2].x = -45.8;
        map->AllOcclusionZone[8].angles[2].y = -5.39;

        map->AllOcclusionZone[8].angles[3].x = -6.8;
        map->AllOcclusionZone[8].angles[3].y = -5.39;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[2].angles[0].x = -4.1;
        map->AllOcclusionZone[2].angles[0].y = 3.5;

        map->AllOcclusionZone[2].angles[1].x = -54.8;
        map->AllOcclusionZone[2].angles[1].y = 3.5;

        map->AllOcclusionZone[2].angles[2].x = -54.8;
        map->AllOcclusionZone[2].angles[2].y = -43;

        map->AllOcclusionZone[2].angles[3].x = -4.1;
        map->AllOcclusionZone[2].angles[3].y = -43;
        ////
        map->AllOcclusionZone[9].angles[0].x = -4.1;
        map->AllOcclusionZone[9].angles[0].y = 3.5;

        map->AllOcclusionZone[9].angles[1].x = -54.8;
        map->AllOcclusionZone[9].angles[1].y = 3.5;

        map->AllOcclusionZone[9].angles[2].x = -54.8;
        map->AllOcclusionZone[9].angles[2].y = -43;

        map->AllOcclusionZone[9].angles[3].x = -4.1;
        map->AllOcclusionZone[9].angles[3].y = -43;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[3].angles[0].x = 36;
        map->AllOcclusionZone[3].angles[0].y = 12.6;

        map->AllOcclusionZone[3].angles[1].x = -9.1;
        map->AllOcclusionZone[3].angles[1].y = 12.6;

        map->AllOcclusionZone[3].angles[2].x = -9.1;
        map->AllOcclusionZone[3].angles[2].y = -34.9;

        map->AllOcclusionZone[3].angles[3].x = 36;
        map->AllOcclusionZone[3].angles[3].y = -34.9;
        ////
        map->AllOcclusionZone[10].angles[0].x = 36;
        map->AllOcclusionZone[10].angles[0].y = 12.6;

        map->AllOcclusionZone[10].angles[1].x = -9.1;
        map->AllOcclusionZone[10].angles[1].y = 12.6;

        map->AllOcclusionZone[10].angles[2].x = -9.1;
        map->AllOcclusionZone[10].angles[2].y = -34.9;

        map->AllOcclusionZone[10].angles[3].x = 36;
        map->AllOcclusionZone[10].angles[3].y = -34.9;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[4].angles[0].x = 15.54;
        map->AllOcclusionZone[4].angles[0].y = 68.4;

        map->AllOcclusionZone[4].angles[1].x = -13;
        map->AllOcclusionZone[4].angles[1].y = 68.4;

        map->AllOcclusionZone[4].angles[2].x = -13;
        map->AllOcclusionZone[4].angles[2].y = 4.43;

        map->AllOcclusionZone[4].angles[3].x = 15.54;
        map->AllOcclusionZone[4].angles[3].y = 4.43;
        ////
        map->AllOcclusionZone[11].angles[0].x = 15.54;
        map->AllOcclusionZone[11].angles[0].y = 68.4;

        map->AllOcclusionZone[11].angles[1].x = -13;
        map->AllOcclusionZone[11].angles[1].y = 68.4;

        map->AllOcclusionZone[11].angles[2].x = -13;
        map->AllOcclusionZone[11].angles[2].y = 4.43;

        map->AllOcclusionZone[11].angles[3].x = 15.54;
        map->AllOcclusionZone[11].angles[3].y = 4.43;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[5].angles[0].x = 58.3;
        map->AllOcclusionZone[5].angles[0].y = 42;

        map->AllOcclusionZone[5].angles[1].x = 29.4;
        map->AllOcclusionZone[5].angles[1].y = 42;

        map->AllOcclusionZone[5].angles[2].x = 29.4;
        map->AllOcclusionZone[5].angles[2].y = -38.1;

        map->AllOcclusionZone[5].angles[3].x = 58.3;
        map->AllOcclusionZone[5].angles[3].y = -38.1;
        ////
        map->AllOcclusionZone[12].angles[0].x = 58.3;
        map->AllOcclusionZone[12].angles[0].y = 42;

        map->AllOcclusionZone[12].angles[1].x = 29.4;
        map->AllOcclusionZone[12].angles[1].y = 42;

        map->AllOcclusionZone[12].angles[2].x = 29.4;
        map->AllOcclusionZone[12].angles[2].y = -38.1;

        map->AllOcclusionZone[12].angles[3].x = 58.3;
        map->AllOcclusionZone[12].angles[3].y = -38.1;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[6].angles[0].x = 37.2;
        map->AllOcclusionZone[6].angles[0].y = 66.2;

        map->AllOcclusionZone[6].angles[1].x = 7.3;
        map->AllOcclusionZone[6].angles[1].y = 66.2;

        map->AllOcclusionZone[6].angles[2].x = 7.3;
        map->AllOcclusionZone[6].angles[2].y = 7.8;

        map->AllOcclusionZone[6].angles[3].x = 37.2;
        map->AllOcclusionZone[6].angles[3].y = 7.8;
        ////
        map->AllOcclusionZone[13].angles[0].x = 37.2;
        map->AllOcclusionZone[13].angles[0].y = 66.2;

        map->AllOcclusionZone[13].angles[1].x = 7.3;
        map->AllOcclusionZone[13].angles[1].y = 66.2;

        map->AllOcclusionZone[13].angles[2].x = 7.3;
        map->AllOcclusionZone[13].angles[2].y = 7.8;

        map->AllOcclusionZone[13].angles[3].x = 37.2;
        map->AllOcclusionZone[13].angles[3].y = 7.8;
    }
    else if (mapToLoad == TUTORIAL)
    {
        // All occlusion culling zones data (a zone will show one or multiple zones)
        map->zonesCount = 2;
        map->AllZones = malloc(map->zonesCount * sizeof(Zone));

        map->AllZones[currentZone].ZoneCount = 2;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 1;
        map->AllZones[currentZone].ZoneCount = 2;
        map->AllZones[currentZone].visibleMapPart[0] = 1;
        map->AllZones[currentZone].visibleMapPart[1] = 0;
        map->AllZones[currentZone].id = currentZone;

        // Add occlusion culling triggers
        CalculateTriggerColBox(0, 2, 22.7, 22.7, 0);
        CalculateTriggerColBox(0, 2, 22.7, 22.7, 1);

        map->shadowCollisionBoxCount = 3;
        map->AllShadowCollisionBox = malloc(map->shadowCollisionBoxCount * sizeof(CollisionBox));

        // Add shadow collision boxes
        CalculateShadowColBox(0, 4.5, 11.65236, 22.7, 9, 3.395271, 0);
        CalculateShadowColBox(7.583056, 4.5, 1.242216, 7.533887, 9, 24.21557, 1);
        CalculateShadowColBox(-1.585769, 1.017356, 1.17581, 2.752997, 2.034714, 2.829585, 2);

        map->occlusionZoneCount = 2;
        map->AllOcclusionZone = malloc(map->occlusionZoneCount * sizeof(OcclusionZone));

        // Add clipping zones
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[0].angles[0].x = 11.335;
        map->AllOcclusionZone[0].angles[0].y = 13.278;

        map->AllOcclusionZone[0].angles[1].x = -11.332;
        map->AllOcclusionZone[0].angles[1].y = 13.278;

        map->AllOcclusionZone[0].angles[2].x = -11.332;
        map->AllOcclusionZone[0].angles[2].y = -9.395;

        map->AllOcclusionZone[0].angles[3].x = 11.335;
        map->AllOcclusionZone[0].angles[3].y = 9.395;

        map->AllOcclusionZone[1].angles[0].x = 11.335;
        map->AllOcclusionZone[1].angles[0].y = 13.278;

        map->AllOcclusionZone[1].angles[1].x = -11.332;
        map->AllOcclusionZone[1].angles[1].y = 13.278;

        map->AllOcclusionZone[1].angles[2].x = -11.332;
        map->AllOcclusionZone[1].angles[2].y = -9.395;

        map->AllOcclusionZone[1].angles[3].x = 11.335;
        map->AllOcclusionZone[1].angles[3].y = 9.395;
    }
    else if (mapToLoad == DUST2_2x2)
    {
        // All occlusion culling zones data (a zone will show one or multiple zones)
        map->zonesCount = 10;
        map->AllZones = malloc(map->zonesCount * sizeof(Zone));

        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 5;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 1;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 2;
        map->AllZones[currentZone].visibleMapPart[3] = 3;
        map->AllZones[currentZone].visibleMapPart[4] = 6;
        map->AllZones[currentZone].visibleMapPart[5] = 7;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 2;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 2;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 6;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 3;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 2;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 4;
        map->AllZones[currentZone].visibleMapPart[3] = 5;
        map->AllZones[currentZone].visibleMapPart[4] = 6;
        map->AllZones[currentZone].visibleMapPart[5] = 7;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 4;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 4;
        map->AllZones[currentZone].visibleMapPart[1] = 5;
        map->AllZones[currentZone].visibleMapPart[2] = 6;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 5;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 4;
        map->AllZones[currentZone].visibleMapPart[1] = 5;
        map->AllZones[currentZone].visibleMapPart[2] = 6;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 6;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 2;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].visibleMapPart[2] = 6;
        map->AllZones[currentZone].visibleMapPart[3] = 7;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 7;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 2;
        map->AllZones[currentZone].visibleMapPart[3] = 3;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 8;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 2;
        map->AllZones[currentZone].visibleMapPart[3] = 3;
        map->AllZones[currentZone].visibleMapPart[4] = 6;
        map->AllZones[currentZone].visibleMapPart[5] = 7;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 9;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 2;
        map->AllZones[currentZone].visibleMapPart[3] = 3;
        map->AllZones[currentZone].visibleMapPart[4] = 6;
        map->AllZones[currentZone].visibleMapPart[5] = 7;
        map->AllZones[currentZone].id = currentZone;

        // Add occlusion culling triggers
        CalculateTriggerColBox(31.25437, -19.58616, 52.12261, 37.01623, 0);
        CalculateTriggerColBox(10.59276, 58.97986, 23.88577, 17.49506, 1);
        CalculateTriggerColBox(22.83467, 41.8413, 16.72676, 16.5031, 2);
        CalculateTriggerColBox(27.63166, 29.56198, 6.307827, 7.687183, 3);
        CalculateTriggerColBox(29.65833, 17.5136, 11.42418, 16.10892, 4);
        CalculateTriggerColBox(47.04734, 20.07397, 22.30289, 42.11788, 5);
        CalculateTriggerColBox(-9.52357, 57.34258, 15.70023, 20.76961, 6);
        CalculateTriggerColBox(11.91049, 2.419476, 25.90689, 6.575387, 7);
        CalculateTriggerColBox(0.2512362, 31.8348, 16.78329, 36.75443, 8);
        CalculateTriggerColBox(3.534717, 9.606822, 9.155338, 8.084706, 9);

        map->shadowCollisionBoxCount = 17;
        map->AllShadowCollisionBox = malloc(map->shadowCollisionBoxCount * sizeof(CollisionBox));

        // Add shadow collision boxes
        CalculateShadowColBox(16.00385, 1.727197, -16.21586, 24.7, 3.42, 14.1, 0);
        CalculateShadowColBox(37.52384, 4.967202, -1.585868, 22.3, 8.46, 24.08, 1);
        CalculateShadowColBox(31.71384, 5.157205, 26.63413, 15.97, 3.98, 15.1, 2);
        CalculateShadowColBox(38.72385, 5.047204, 31.65415, 5.91, 3.62, 6.38, 3);
        CalculateShadowColBox(50.48385, 3.157205, 35.40415, 13, 10.9, 11.83, 4);
        CalculateShadowColBox(26.97384, 6.007195, 15.57414, 6.1, 6.05, 11.29, 5);
        CalculateShadowColBox(38.82384, 8.397195, -33.39587, 4.38, 4.07, 8.44, 6);
        CalculateShadowColBox(19.78384, 7.157205, -14.67586, 4.89, 7.2, 28.1, 7);
        CalculateShadowColBox(18.25384, 6.437203, 5.844155, 16.05, 7.2, 5.4, 8);
        CalculateShadowColBox(-2.746141, 4.257195, 16.90415, 10.61, 11, 36.6, 9);
        CalculateShadowColBox(-2.436144, 8.457208, 66.10413, 29.28, 11.4, 2.86, 10);
        CalculateShadowColBox(17.28386, 5.877206, 63.06413, 14.3, 6.7, 4.37, 11);
        CalculateShadowColBox(29.67384, 5.877206, 49.91413, 14.3, 6.7, 4.37, 12);
        CalculateShadowColBox(12.53386, 5.877206, 40.81413, 14.3, 6.7, 21, 13);
        CalculateShadowColBox(-3.626146, 5.877206, 40.73414, 12.42, 6.7, 31.4, 14);
        CalculateShadowColBox(-9.356157, 8.267205, 57.86414, 4.4, 5.6, 17.13, 15);
        CalculateShadowColBox(39.72386, 6.616204, -19.41586, 1.99, 2.22, 3.34, 16);

        map->occlusionZoneCount = 8;
        map->AllOcclusionZone = malloc(map->occlusionZoneCount * sizeof(OcclusionZone));

        // Add clipping zones
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[0].angles[0].x = 36;
        map->AllOcclusionZone[0].angles[0].y = 12.6;

        map->AllOcclusionZone[0].angles[1].x = -9.1;
        map->AllOcclusionZone[0].angles[1].y = 12.6;

        map->AllOcclusionZone[0].angles[2].x = -9.1;
        map->AllOcclusionZone[0].angles[2].y = -34.9;

        map->AllOcclusionZone[0].angles[3].x = 36;
        map->AllOcclusionZone[0].angles[3].y = -34.9;

        map->AllOcclusionZone[1].angles[0].x = 36;
        map->AllOcclusionZone[1].angles[0].y = 12.6;

        map->AllOcclusionZone[1].angles[1].x = -9.1;
        map->AllOcclusionZone[1].angles[1].y = 12.6;

        map->AllOcclusionZone[1].angles[2].x = -9.1;
        map->AllOcclusionZone[1].angles[2].y = -34.9;

        map->AllOcclusionZone[1].angles[3].x = 36;
        map->AllOcclusionZone[1].angles[3].y = -34.9;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[2].angles[0].x = 15.54;
        map->AllOcclusionZone[2].angles[0].y = 68.4;

        map->AllOcclusionZone[2].angles[1].x = -13;
        map->AllOcclusionZone[2].angles[1].y = 68.4;

        map->AllOcclusionZone[2].angles[2].x = -13;
        map->AllOcclusionZone[2].angles[2].y = 4.43;

        map->AllOcclusionZone[2].angles[3].x = 15.54;
        map->AllOcclusionZone[2].angles[3].y = 4.43;

        map->AllOcclusionZone[3].angles[0].x = 15.54;
        map->AllOcclusionZone[3].angles[0].y = 68.4;

        map->AllOcclusionZone[3].angles[1].x = -13;
        map->AllOcclusionZone[3].angles[1].y = 68.4;

        map->AllOcclusionZone[3].angles[2].x = -13;
        map->AllOcclusionZone[3].angles[2].y = 4.43;

        map->AllOcclusionZone[3].angles[3].x = 15.54;
        map->AllOcclusionZone[3].angles[3].y = 4.43;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[4].angles[0].x = 58.3;
        map->AllOcclusionZone[4].angles[0].y = 42;

        map->AllOcclusionZone[4].angles[1].x = 29.4;
        map->AllOcclusionZone[4].angles[1].y = 42;

        map->AllOcclusionZone[4].angles[2].x = 29.4;
        map->AllOcclusionZone[4].angles[2].y = -38.1;

        map->AllOcclusionZone[4].angles[3].x = 58.3;
        map->AllOcclusionZone[4].angles[3].y = -38.1;

        map->AllOcclusionZone[5].angles[0].x = 58.3;
        map->AllOcclusionZone[5].angles[0].y = 42;

        map->AllOcclusionZone[5].angles[1].x = 29.4;
        map->AllOcclusionZone[5].angles[1].y = 42;

        map->AllOcclusionZone[5].angles[2].x = 29.4;
        map->AllOcclusionZone[5].angles[2].y = -38.1;

        map->AllOcclusionZone[5].angles[3].x = 58.3;
        map->AllOcclusionZone[5].angles[3].y = -38.1;
        /////////////////////////////////////////////////////
        map->AllOcclusionZone[6].angles[0].x = 37.2;
        map->AllOcclusionZone[6].angles[0].y = 66.2;

        map->AllOcclusionZone[6].angles[1].x = 7.3;
        map->AllOcclusionZone[6].angles[1].y = 66.2;

        map->AllOcclusionZone[6].angles[2].x = 7.3;
        map->AllOcclusionZone[6].angles[2].y = 7.8;

        map->AllOcclusionZone[6].angles[3].x = 37.2;
        map->AllOcclusionZone[6].angles[3].y = 7.8;

        map->AllOcclusionZone[7].angles[0].x = 37.2;
        map->AllOcclusionZone[7].angles[0].y = 66.2;

        map->AllOcclusionZone[7].angles[1].x = 7.3;
        map->AllOcclusionZone[7].angles[1].y = 66.2;

        map->AllOcclusionZone[7].angles[2].x = 7.3;
        map->AllOcclusionZone[7].angles[2].y = 7.8;

        map->AllOcclusionZone[7].angles[3].x = 37.2;
        map->AllOcclusionZone[7].angles[3].y = 7.8;
    }
    else if (mapToLoad == AIM_MAP)
    {
        // All occlusion culling zones data (a zone will show one or multiple zones)
        map->zonesCount = 2;
        map->AllZones = malloc(map->zonesCount * sizeof(Zone));

        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 2;
        map->AllZones[currentZone].visibleMapPart[3] = 3;
        map->AllZones[currentZone].visibleMapPart[4] = 4;
        map->AllZones[currentZone].visibleMapPart[5] = 5;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 1;
        map->AllZones[currentZone].ZoneCount = 6;
        map->AllZones[currentZone].visibleMapPart[0] = 5;
        map->AllZones[currentZone].visibleMapPart[1] = 4;
        map->AllZones[currentZone].visibleMapPart[2] = 3;
        map->AllZones[currentZone].visibleMapPart[3] = 2;
        map->AllZones[currentZone].visibleMapPart[4] = 1;
        map->AllZones[currentZone].visibleMapPart[5] = 0;
        map->AllZones[currentZone].id = currentZone;

        // Add occlusion culling triggers
        CalculateTriggerColBox(0, 0, 34, 60, 0);
        CalculateTriggerColBox(0, 0, 34, 60, 1);

        map->shadowCollisionBoxCount = 6;
        map->AllShadowCollisionBox = malloc(map->shadowCollisionBoxCount * sizeof(CollisionBox));

        // Add shadow collision boxes
        CalculateShadowColBox(15.21, 0, 0.3, 1.92, 9, 35.7, 0);
        CalculateShadowColBox(12.73, 1.38, -22.63, 2.752997, 2.9, 2.829585, 1);
        CalculateShadowColBox(13.53, 1.38, -20.29, 2.752997, 2.9, 2.829585, 2);
        CalculateShadowColBox(15.14, 1.38, -18.5, 2.752997, 2.9, 2.829585, 3);
        CalculateShadowColBox(15.93, 4.28, -22.99, 1.12, 2.9, 12.29, 4);
        CalculateShadowColBox(15.66, 3.7, 23.33, 1.12, 7.69, 12.1, 5);

        map->occlusionZoneCount = 6;
        map->AllOcclusionZone = malloc(map->occlusionZoneCount * sizeof(OcclusionZone));

        // Add clipping zones
        ///////////////////////////////////////////////////// Y must be inverted compared to Unity project
        map->AllOcclusionZone[0].angles[0].x = 16.2;
        map->AllOcclusionZone[0].angles[0].y = 23;

        map->AllOcclusionZone[0].angles[1].x = -16.2;
        map->AllOcclusionZone[0].angles[1].y = 23;

        map->AllOcclusionZone[0].angles[2].x = -16.2;
        map->AllOcclusionZone[0].angles[2].y = -23.5;

        map->AllOcclusionZone[0].angles[3].x = 16.2;
        map->AllOcclusionZone[0].angles[3].y = -23.5;

        map->AllOcclusionZone[1].angles[0].x = 16.2;
        map->AllOcclusionZone[1].angles[0].y = 23;

        map->AllOcclusionZone[1].angles[1].x = -16.2;
        map->AllOcclusionZone[1].angles[1].y = 23;

        map->AllOcclusionZone[1].angles[2].x = -16.2;
        map->AllOcclusionZone[1].angles[2].y = -23.5;

        map->AllOcclusionZone[1].angles[3].x = 16.2;
        map->AllOcclusionZone[1].angles[3].y = -23.5;

        //////////////////////////////////////////////////

        map->AllOcclusionZone[2].angles[0].x = 16.5;
        map->AllOcclusionZone[2].angles[0].y = -22.5;

        map->AllOcclusionZone[2].angles[1].x = -16.5;
        map->AllOcclusionZone[2].angles[1].y = -22.5;

        map->AllOcclusionZone[2].angles[2].x = -16.5;
        map->AllOcclusionZone[2].angles[2].y = -35.6;

        map->AllOcclusionZone[2].angles[3].x = 16.5;
        map->AllOcclusionZone[2].angles[3].y = -35.6;

        map->AllOcclusionZone[3].angles[0].x = 16.5;
        map->AllOcclusionZone[3].angles[0].y = -22.5;

        map->AllOcclusionZone[3].angles[1].x = -16.5;
        map->AllOcclusionZone[3].angles[1].y = -22.5;

        map->AllOcclusionZone[3].angles[2].x = -16.5;
        map->AllOcclusionZone[3].angles[2].y = -35.6;

        map->AllOcclusionZone[3].angles[3].x = 16.5;
        map->AllOcclusionZone[3].angles[3].y = -35.6;

        ////////////////////////////////////////////////

        map->AllOcclusionZone[4].angles[0].x = 16.5;
        map->AllOcclusionZone[4].angles[0].y = 35.6;

        map->AllOcclusionZone[4].angles[1].x = -16.5;
        map->AllOcclusionZone[4].angles[1].y = 35.6;

        map->AllOcclusionZone[4].angles[2].x = -16.5;
        map->AllOcclusionZone[4].angles[2].y = 22.2;

        map->AllOcclusionZone[4].angles[3].x = 16.5;
        map->AllOcclusionZone[4].angles[3].y = 22.2;

        map->AllOcclusionZone[5].angles[0].x = 16.5;
        map->AllOcclusionZone[5].angles[0].y = 35.6;

        map->AllOcclusionZone[5].angles[1].x = -16.5;
        map->AllOcclusionZone[5].angles[1].y = 35.6;

        map->AllOcclusionZone[5].angles[2].x = -16.5;
        map->AllOcclusionZone[5].angles[2].y = 22.2;

        map->AllOcclusionZone[5].angles[3].x = 16.5;
        map->AllOcclusionZone[5].angles[3].y = 22.2;

    }
    else if (mapToLoad == B2000)
    {
        // All occlusion culling zones data (a zone will show one or multiple zones)
        map->zonesCount = 2;
        map->AllZones = malloc(map->zonesCount * sizeof(Zone));

        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].visibleMapPart[2] = 2;
        map->AllZones[currentZone].visibleMapPart[3] = 3;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 1;
        map->AllZones[currentZone].ZoneCount = 4;
        map->AllZones[currentZone].visibleMapPart[0] = 3;
        map->AllZones[currentZone].visibleMapPart[1] = 2;
        map->AllZones[currentZone].visibleMapPart[2] = 1;
        map->AllZones[currentZone].visibleMapPart[3] = 0;
        map->AllZones[currentZone].id = currentZone;

        // Add occlusion culling triggers
        CalculateTriggerColBox(0, 0, 42, 48, 0);
        CalculateTriggerColBox(0, 0, 42, 48, 1);

        map->shadowCollisionBoxCount = 18;
        map->AllShadowCollisionBox = malloc(map->shadowCollisionBoxCount * sizeof(CollisionBox));

        // Add shadow collision boxes
        CalculateShadowColBox(19.51, 2.77, 0.17, 4.64, 9, 46.4, 0);
        CalculateShadowColBox(-18.55, 2.77, 0.17, 4.75, 9, 46.4, 1);
        CalculateShadowColBox(0.37, 2.77, 20.2, 37.92, 9, 5.81, 2);
        CalculateShadowColBox(0.28, 2.77, -21.55, 37.92, 9, 3.25, 3);
        CalculateShadowColBox(-15.2, 2.77, -17.49, 11.37, 9, 6.8, 4);
        CalculateShadowColBox(15.6, 2.77, -17.49, 9.5, 9, 6.8, 5);
        CalculateShadowColBox(0.75, 2.77, -9.96, 25.2, 9, 8.1, 6);
        CalculateShadowColBox(0.71, 2.77, 7.4, 25.2, 9, 7.44, 7);
        CalculateShadowColBox(-15.28, 2.77, 14.72, 10.67, 9, 6.8, 8);
        CalculateShadowColBox(15.59, 2.77, 14.72, 9.5, 9, 6.8, 9);
        CalculateShadowColBox(10.22, 2.77, -1.09, 5.69, 9, 9.9, 10);
        CalculateShadowColBox(-9.099999, 2.77, -1.09, 5.61, 9, 9.9, 11);
        CalculateShadowColBox(-2.51, 2.77, -1.09, 4.45, 9, 9.9, 12);
        CalculateShadowColBox(3.65, 2.77, -1.09, 4.15, 9, 9.9, 13);
        CalculateShadowColBox(0.6799999, 2.77, 1.91, 1.74, 9, 3.83, 14);
        CalculateShadowColBox(0.6799999, 2.77, -4.33, 1.74, 9, 3.83, 15);
        CalculateShadowColBox(-5.51, 2.77, -2.98, 1.74, 9, 3.83, 16);
        CalculateShadowColBox(6.5, 2.77, -2.98, 1.74, 9, 3.83, 17);

        map->occlusionZoneCount = 4;
        map->AllOcclusionZone = malloc(map->occlusionZoneCount * sizeof(OcclusionZone));

        // Add clipping zones
        ///////////////////////////////////////////////////// Y must be inverted compared to Unity project

        map->AllOcclusionZone[0].angles[0].x = 23;
        map->AllOcclusionZone[0].angles[0].y = 5;

        map->AllOcclusionZone[0].angles[1].x = -23;
        map->AllOcclusionZone[0].angles[1].y = 5;

        map->AllOcclusionZone[0].angles[2].x = -23;
        map->AllOcclusionZone[0].angles[2].y = -25;

        map->AllOcclusionZone[0].angles[3].x = 23;
        map->AllOcclusionZone[0].angles[3].y = -25;

        map->AllOcclusionZone[1].angles[0].x = 23;
        map->AllOcclusionZone[1].angles[0].y = 5;

        map->AllOcclusionZone[1].angles[1].x = -23;
        map->AllOcclusionZone[1].angles[1].y = 5;

        map->AllOcclusionZone[1].angles[2].x = -23;
        map->AllOcclusionZone[1].angles[2].y = -25;

        map->AllOcclusionZone[1].angles[3].x = 23;
        map->AllOcclusionZone[1].angles[3].y = -25;

        //////////////////////////////////////////////////

        map->AllOcclusionZone[2].angles[0].x = 23;
        map->AllOcclusionZone[2].angles[0].y = 25;

        map->AllOcclusionZone[2].angles[1].x = -23;
        map->AllOcclusionZone[2].angles[1].y = 25;

        map->AllOcclusionZone[2].angles[2].x = -23;
        map->AllOcclusionZone[2].angles[2].y = -5;

        map->AllOcclusionZone[2].angles[3].x = 23;
        map->AllOcclusionZone[2].angles[3].y = -5;

        map->AllOcclusionZone[3].angles[0].x = 23;
        map->AllOcclusionZone[3].angles[0].y = 25;

        map->AllOcclusionZone[3].angles[1].x = -23;
        map->AllOcclusionZone[3].angles[1].y = 25;

        map->AllOcclusionZone[3].angles[2].x = -23;
        map->AllOcclusionZone[3].angles[2].y = -5;

        map->AllOcclusionZone[3].angles[3].x = 23;
        map->AllOcclusionZone[3].angles[3].y = -5;
    }
    else if (mapToLoad == MIRAGEA)
    {
        // All occlusion culling zones data (a zone will show one or multiple zones)
        map->zonesCount = 5;
        map->AllZones = malloc(map->zonesCount * sizeof(Zone));

        map->AllZones[currentZone].ZoneCount = 2;
        map->AllZones[currentZone].visibleMapPart[0] = 0; //1 = tspawn, 2 = site, 3 ct spawn
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 1;
        map->AllZones[currentZone].ZoneCount = 2;
        map->AllZones[currentZone].visibleMapPart[0] = 4;
        map->AllZones[currentZone].visibleMapPart[1] = 5;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 2;
        map->AllZones[currentZone].ZoneCount = 2;
        map->AllZones[currentZone].visibleMapPart[0] = 2;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 3;
        map->AllZones[currentZone].ZoneCount = 2;
        map->AllZones[currentZone].visibleMapPart[0] = 0;
        map->AllZones[currentZone].visibleMapPart[1] = 1;
        map->AllZones[currentZone].id = currentZone;

        currentZone = 4;
        map->AllZones[currentZone].ZoneCount = 2;
        map->AllZones[currentZone].visibleMapPart[0] = 2;
        map->AllZones[currentZone].visibleMapPart[1] = 3;
        map->AllZones[currentZone].id = currentZone;


        // Add occlusion culling triggers
        CalculateTriggerColBox(-12.3, 38.6, 65, 35, 0);
        CalculateTriggerColBox(-23.9, -32.3, 51, 29.84, 1);
        CalculateTriggerColBox(-30.2, -1.1, 39.22, 32.3, 2);
        CalculateTriggerColBox(-40.4, 15.64, 8, 1, 3);
        CalculateTriggerColBox(-20.12, 20.52, 10, 1, 4);

        map->shadowCollisionBoxCount = 12;
        map->AllShadowCollisionBox = malloc(map->shadowCollisionBoxCount * sizeof(CollisionBox));

        // Add shadow collision boxes
        CalculateShadowColBox(15.26385, 3.877197, 33.48413, 39.3, 7.9, 16.4, 0);
        CalculateShadowColBox(-14.00616, 2.267197, 40.72415, 14.19, 5.58, 14.1, 1);
        CalculateShadowColBox(-36.78616, 6.637207, 24.82413, 9.2, 6.6, 28.18, 2);
        CalculateShadowColBox(-30.24615, 4.657196, 34.68414, 20.8, 3.42, 12.9, 3);
        CalculateShadowColBox(-31.59616, 3.277206, 12.26413, 7.82, 3.42, 4.98, 4);
        CalculateShadowColBox(-41.82614, 5.847198, 1.774139, 7.82, 7.26, 20.7, 5);
        CalculateShadowColBox(-23.25616, 5.847198, -11.69586, 21.59, 7.26, 5.79, 6);
        CalculateShadowColBox(-44.39615, 3.5672, -21.27585, 7.82, 8.9, 27.48, 7);
        CalculateShadowColBox(-38.27615, 3.5672, -35.83585, 20.9, 8.9, 13.14, 8);
        CalculateShadowColBox(-7.886139, 3.5672, -34.79587, 25.9, 8.9, 7.86, 9);
        CalculateShadowColBox(-8.586151, 6.707199, -20.90585, 21.93, 8.9, 7.62, 10);
        CalculateShadowColBox(-14.96616, 4.137207, 22.34415, 19.5, 8.4, 12, 11);

        map->occlusionZoneCount = 6;
        map->AllOcclusionZone = malloc(map->occlusionZoneCount * sizeof(OcclusionZone));

        // Add clipping zones
        ///////////////////////////////////////////////////// Y must be inverted compared to Unity project

        map->AllOcclusionZone[0].angles[0].x = 40;
        map->AllOcclusionZone[0].angles[0].y = 70;

        map->AllOcclusionZone[0].angles[1].x = -42;
        map->AllOcclusionZone[0].angles[1].y = 70;

        map->AllOcclusionZone[0].angles[2].x = -42;
        map->AllOcclusionZone[0].angles[2].y = 13;

        map->AllOcclusionZone[0].angles[3].x = 40;
        map->AllOcclusionZone[0].angles[3].y = 13;

        //shad

        map->AllOcclusionZone[1].angles[0].x = 40;
        map->AllOcclusionZone[1].angles[0].y = 70;

        map->AllOcclusionZone[1].angles[1].x = -42;
        map->AllOcclusionZone[1].angles[1].y = 70;

        map->AllOcclusionZone[1].angles[2].x = -42;
        map->AllOcclusionZone[1].angles[2].y = 13;

        map->AllOcclusionZone[1].angles[3].x = 40;
        map->AllOcclusionZone[1].angles[3].y = 13;

        ///////////////////////////////////////////////////

        map->AllOcclusionZone[2].angles[0].x = -2.5;
        map->AllOcclusionZone[2].angles[0].y = 24;

        map->AllOcclusionZone[2].angles[1].x = -46;
        map->AllOcclusionZone[2].angles[1].y = 24;

        map->AllOcclusionZone[2].angles[2].x = -46;
        map->AllOcclusionZone[2].angles[2].y = -30;

        map->AllOcclusionZone[2].angles[3].x = 2.5;
        map->AllOcclusionZone[2].angles[3].y = -30;

        //shad

        map->AllOcclusionZone[3].angles[0].x = -2.5;
        map->AllOcclusionZone[3].angles[0].y = 24;

        map->AllOcclusionZone[3].angles[1].x = -46;
        map->AllOcclusionZone[3].angles[1].y = 24;

        map->AllOcclusionZone[3].angles[2].x = -46;
        map->AllOcclusionZone[3].angles[2].y = -30;

        map->AllOcclusionZone[3].angles[3].x = 2.5;
        map->AllOcclusionZone[3].angles[3].y = -30;

        ///////////////////////////////////////////////////

        map->AllOcclusionZone[4].angles[0].x = 15;
        map->AllOcclusionZone[4].angles[0].y = -16;

        map->AllOcclusionZone[4].angles[1].x = -47;
        map->AllOcclusionZone[4].angles[1].y = -16;

        map->AllOcclusionZone[4].angles[2].x = -47;
        map->AllOcclusionZone[4].angles[2].y = -60;

        map->AllOcclusionZone[4].angles[3].x = 15;
        map->AllOcclusionZone[4].angles[3].y = -60;

        //shad

        map->AllOcclusionZone[5].angles[0].x = 15;
        map->AllOcclusionZone[5].angles[0].y = -16;

        map->AllOcclusionZone[5].angles[1].x = -47;
        map->AllOcclusionZone[5].angles[1].y = -16;

        map->AllOcclusionZone[5].angles[2].x = -47;
        map->AllOcclusionZone[5].angles[2].y = -60;

        map->AllOcclusionZone[5].angles[3].x = 15;
        map->AllOcclusionZone[5].angles[3].y = -60;

    }
    

    // Convert map limits to a int format
    for (int i = 0; i < map->occlusionZoneCount; i++)
    {
        for (int i2 = 0; i2 < 4; i2++)
        {
            map->AllOcclusionZone[i].anglesInt[i2].x = map->AllOcclusionZone[i].angles[i2].x * 4096.0;
            map->AllOcclusionZone[i].anglesInt[i2].y = map->AllOcclusionZone[i].angles[i2].y * 4096.0;
        }
    }
}

/**
 * @brief Add walls for collisions for dust2 map.
 * Generated with the Unity project
 */
void LoadAllCollisions(int mapToLoad)
{
    if (mapToLoad == DUST2)
    {
        mapToSet = DUST2;
        allMaps[mapToLoad].CollisionsCount = 202;
        allMaps[mapToLoad].AllWallsCollisions = malloc(allMaps[mapToLoad].CollisionsCount * sizeof(Wall));
        CreateWall(9.38846, -1.4, -8.007592, 35.77919, 1, 33.08558, 3, 0);
        CreateWall(3.991, 0.285, 4.786, 2.423, 2.423, 2.423, 3, 1);
        CreateWall(5.41, 1.107, 5.980854, 0.4, 4, 30.68171, 3, 2);
        CreateWall(-1.4, 1.8, 38.985, 13, 1, 34.67, 4, 3);
        CreateWall(-10.31409, -1.03, 5.18, 17.45819, 1, 7, 1, 4);
        CreateWall(-1.654, 2.288648, 16.47623, 0.8, 6.477295, 19.37246, 4, 5);
        CreateWall(-1.654, 2.05, 2.617579, 0.8, 6, 1.884841, 3, 6);
        CreateWall(24.42, 1.8, 29.52468, 37.5, 1, 67.90935, -1, 7);
        CreateWall(48.3451, 1.8, 1.665, 9.130207, 1, 38.67, 5, 8);
        CreateWall(40.63582, 1.8, -14.36, 5.668356, 1, 7, 5, 9);
        CreateWall(44.5, 5.04, -33.965, 15, 1, 6.67, 5, 10);
        CreateWall(47.75, -3.04, 37.732, 9, 1, 7, 5, 11);
        CreateWall(40.68, 4.23, -23.46, 5.7, 1, 11.2, 5, 12);
        CreateWall(27.58, 4.225, -24.69, 20, 1, 9, 3, 13);
        CreateWall(21.51, 4.225, -12.405, 7, 1, 15.17, 3, 14);
        CreateWall(-30.05, 1.8, -16.65, 24.67, 1, 34, 2, 15);
        CreateWall(-37.12021, 2.6, -30.795, 9.679577, 1, 19.67, 2, 16);
        CreateWall(-38.66, 2.6065, 3.923, 10, 1, 16.17, 1, 17);
        CreateWall(-26.975, 2.6065, 10.75, 12.67, 1, 11.5, 1, 18);
        CreateWall(-31.85, 2.6065, 21.645, 16.8, 1, 9.97, 1, 19);
        CreateWall(-31.85, 1.8, 35.88, 16.8, 1, 16, 0, 20);
        CreateWall(-36.76, 5.03, 60.475, 6.5, 1, 6.67, 0, 21);
        CreateWall(-42.78, 5.03, 58.6, 5, 1, 10, 0, 22);
        CreateWall(-22.91243, 5.03, 53.76, 20.59513, 1, 19, 0, 23);
        CreateWall(-7.165, 5.03, 61.57, 11.17, 1, 10, 4, 24);
        CreateWall(-15.974, 0.591, 13.66, 3.7, 1, 3.37, 1, 25);
        CreateWall(55.00196, 3.415, 23.80224, 3.714117, 1, 17.83263, 5, 26);
        CreateWall(4.846629, 2.989377, -0.481, 0.9097431, 7.831755, 2.423, 3, 27);
        CreateWall(-0.8933716, 2.709331, -0.481, 0.9097431, 7.271662, 2.423, 3, 28);
        CreateWall(0.6405261, 1.114704, 0.3353194, 2.150795, 4.082407, 0.6293912, 3, 29);
        CreateWall(3.354151, 1.114704, -1.250537, 2.145238, 4.082407, 0.6865859, 3, 30);
        CreateWall(4.053317, 1.114704, -0.5631354, 0.6353655, 4.082407, 0.3297293, 3, 31);
        CreateWall(-0.1147662, 1.114704, -0.41167, 0.6778869, 4.082407, 0.3050682, 3, 32);
        CreateWall(-10.94747, 2.889332, -4.495358, 19.35818, 7.631665, 12.90928, -1, 33);
        CreateWall(-1.26794, 2.889332, -23.5255, 12.94932, 7.631665, 2.408997, 3, 34);
        CreateWall(-8.63794, 4.209399, -25.88165, 8.13, 10.2718, 3.561691, 2, 35);
        CreateWall(-15.26169, 4.209399, -28.2181, 11.56, 10.2718, 1.161465, 2, 36);
        CreateWall(-16.24241, 3.05695, -12.16243, 2.29025, 3.088099, 2.408759, 2, 37);
        CreateWall(-20.628, 4.339704, -12.831, 0.6293912, 4.082407, 2.150795, 2, 38);
        CreateWall(-21.486, 4.339704, -16.268, 0.3297293, 4.082407, 0.6353655, 2, 39);
        CreateWall(-22.215, 4.339704, -15.521, 0.6293912, 4.082407, 2.150795, 2, 40);
        CreateWall(-21.352, 4.339704, -12.076, 0.3297293, 4.082407, 0.6353655, 2, 41);
        CreateWall(-17.98957, 2.909, -11.5547, 1.207145, 1.2, 1.211931, 2, 42);
        CreateWall(-21.42, 5.894331, -11.297, 2.423, 7.271662, 0.9097431, 2, 43);
        CreateWall(-21.42, 5.894331, -17.06, 2.423, 7.271662, 0.9097431, 2, 44);
        CreateWall(-21.41365, 5.894331, -21.33369, 1.604465, 7.271662, 8.111914, 2, 45);
        CreateWall(-21.41365, 5.894331, -8.338291, 1.604465, 7.271662, 5.283774, 2, 46);
        CreateWall(-21.41365, 5.894331, -29.9949, 1.604465, 7.271662, 4.181724, 2, 47);
        CreateWall(-17.20958, 4.082152, -26.64717, 9.35, 3.647305, 2.424164, 2, 48);
        CreateWall(-23.02708, 3.912082, -27.25182, 1.616625, 3.231766, 1.284817, 2, 49);
        CreateWall(-23.02901, 4.519009, -25.66659, 1.620493, 1.219922, 1.21636, 2, 50);
        CreateWall(-23.8313, 3.102336, -25.47028, 3.225069, 1.615268, 1.608994, 2, 51);
        CreateWall(9.870321, 1.493199, -8.82, 8.488289, 4.839397, 1, 3, 52);
        CreateWall(19.71741, 1.488101, -10.13818, 11.26746, 4.849592, 1.627839, 3, 53);
        CreateWall(34.23705, 3.871293, -0.4494247, 19.34229, 9.615976, 20.9745, -1, 54);
        CreateWall(8.829233, 0.6898834, -10.13725, 1.602546, 3.232767, 1.615795, 3, 55);
        CreateWall(7.217544, 0.2866741, -16.19459, 2.415168, 2.426349, 2.424468, 3, 56);
        CreateWall(6.816122, 2.310524, -16.59494, 1.612324, 1.596437, 1.616609, 3, 57);
        CreateWall(18.91403, 1.488101, -23.54818, 11.28471, 4.849592, 1, 3, 58);
        CreateWall(9.246572, 1.488101, -20.25092, 8.058916, 4.849592, 5.70549, 3, 59);
        CreateWall(24.96741, 1.488101, -20.2333, 0.8, 4.849592, 5.66225, 3, 60);
        CreateWall(26.58551, 0.4672852, -18.67, 2.418206, 2.457632, 2.39, 3, 61);
        CreateWall(29.00651, 1.064285, -18.67, 2.418206, 2.457632, 2.39, 3, 62);
        CreateWall(26.98624, 2.499154, -19.01415, 1.618732, 1.627894, 1.629699, 3, 63);
        CreateWall(31.60709, 2.795956, -20.02006, 12.55215, 5.453778, 0.4001274, 3, 64);
        CreateWall(40.56831, 3.903672, -17.60006, 6.194584, 3.238344, 0.4001274, 5, 65);
        CreateWall(37.69, 3.903672, -18.81139, 0.4001274, 3.238344, 2.822779, 5, 66);
        CreateWall(43.73696, 3.752151, -23.85887, 0.4140491, 3.541387, 12.91774, 5, 67);
        CreateWall(25.17, 4.305497, -15.58066, 0.4001274, 2.434694, 9.287472, 3, 68);
        CreateWall(38.68263, 5.510857, -19.60757, 1.611956, 1.60449, 3.610546, 5, 69);
        CreateWall(38.68263, 7.150856, -19.01406, 1.611956, 1.60449, 1.619928, 5, 70);
        CreateWall(42.71363, 5.530857, -22.65006, 1.611956, 1.60449, 1.619928, 5, 71);
        CreateWall(27.79171, 7.936794, -32.73777, 19.3618, 6.416366, 8.091341, 3, 72);
        CreateWall(27.78882, 7.936794, -28.30295, 7.260923, 6.416366, 0.8257637, 3, 73);
        CreateWall(15.6907, 7.936794, -13.78543, 4.817293, 6.416366, 29.87009, 3, 74);
        CreateWall(18.42, 7.936794, -20.63499, 1, 6.416366, 7.24859, 3, 75);
        CreateWall(44.73563, 3.656087, -19.84385, 1.611956, 2.038029, 1.555517, 5, 76);
        CreateWall(54.45702, 6.858232, -27.74011, 4.934025, 8.573491, 20.66022, 5, 77);
        CreateWall(44.79, 6.858232, -37.25, 15, 8.573491, 1, 5, 78);
        CreateWall(43.12514, 3.871293, -10.13021, 3.204861, 9.615976, 3.212919, 5, 79);
        CreateWall(43.12514, 3.871293, 9.217745, 3.204861, 9.615976, 3.228837, 5, 80);
        CreateWall(55.61611, 4.50092, -5.702986, 2.438217, 2.458159, 2.414028, 5, 81);
        CreateWall(55.15685, 5.471698, 5.165548, 6.338552, 6.415167, 19.35178, 5, 82);
        CreateWall(58.07824, 5.471698, 7.948711, 2.521341, 6.415167, 50.76546, 5, 83);
        CreateWall(56.02677, 4.719615, 15.67324, 1.615543, 1.62077, 1.627525, 5, 84);
        CreateWall(55.63088, 6.345802, 31.40095, 2.447762, 4.873144, 2.418945, 5, 85);
        CreateWall(47.67071, 6.720139, 36.50803, 13.9532, 8.840049, 7.817391, 5, 86);
        CreateWall(37.47268, 6.720139, 35.01363, 3.218556, 8.840049, 1.644658, 5, 87);
        CreateWall(38.30424, 6.720139, 36.66363, 4.881684, 8.840049, 1.644658, 5, 88);
        CreateWall(47.88821, -0.1699834, 41.53085, 8.275005, 4.7418, 1.703053, 5, 89);
        CreateWall(43.52215, 0.2628939, 31.01274, 0.8107128, 5.686295, 19.40301, 5, 90);
        CreateWall(44.72398, -1.73808, 39.10096, 1.634037, 1.618159, 3.295925, 5, 91);
        CreateWall(51.18998, -1.73808, 35.84166, 1.634037, 1.618159, 3.231308, 5, 92);
        CreateWall(51.18998, -0.1290795, 36.23938, 1.634037, 1.618159, 1.610428, 5, 93);
        CreateWall(52.32014, 0.8512607, 29.83927, 0.6802864, 6.955806, 20.27887, 5, 94);
        CreateWall(52.32679, 3.286222, 15.67575, 0.6669998, 2.08588, 1.603443, 5, 95);
        CreateWall(33.44603, 6.720139, 36.03182, 4.826495, 8.840049, 29.46829, 6, 96);
        CreateWall(22.95895, 6.720139, 21.99475, 3.240649, 8.840049, 24.45416, 6, 97);
        CreateWall(30.64663, 5.934331, 33.42, 0.9097431, 7.271662, 2.423, 6, 98);
        CreateWall(24.90663, 5.934331, 33.42, 0.9097431, 7.271662, 2.423, 6, 99);
        CreateWall(26.44053, 4.339704, 32.63532, 2.150795, 4.082407, 0.6293912, 6, 100);
        CreateWall(29.15415, 4.339704, 34.19246, 2.145238, 4.082407, 0.6865859, 6, 101);
        CreateWall(29.85332, 4.339704, 33.49086, 0.6353655, 4.082407, 0.3297293, 6, 102);
        CreateWall(25.68523, 4.339704, 33.37033, 0.6778869, 4.082407, 0.3050682, 6, 103);
        CreateWall(29.813, 3.51, 24.547, 2.423, 2.423, 2.423, 6, 104);
        CreateWall(24.90663, 5.934331, 22.13, 0.9097431, 7.271662, 2.423, 6, 105);
        CreateWall(30.64663, 5.934331, 22.13, 0.9097431, 7.271662, 2.423, 6, 106);
        CreateWall(26.37633, 6.720139, 57.01281, 10.12095, 8.840049, 13.31212, 6, 107);
        CreateWall(15.38498, 6.720139, 65.11506, 13.95091, 8.840049, 3.702241, 6, 108);
        CreateWall(-18.70116, 6.720139, 67.90365, 54.33317, 8.840049, 2.839417, 0, 109);
        CreateWall(-20.62166, 9.071501, 66.18, 7.271675, 5.883001, 1, 0, 110);
        CreateWall(-33.51166, 9.071501, 66.18, 7.271675, 5.883001, 1, 0, 111);
        CreateWall(19.46931, 6.720139, 16.87856, 9.941291, 8.840049, 23.40178, -1, 112);
        CreateWall(11.42511, 6.720139, 16.45395, 6.092877, 8.840049, 22.55255, -1, 113);
        CreateWall(8.43699, 6.720139, 43.91084, 12.90911, 8.840049, 19.34007, -1, 114);
        CreateWall(15.2, 6.720139, 43.90626, 1, 8.840049, 7.272514, 6, 115);
        CreateWall(9.832866, 6.720139, 33.91, 10.07427, 8.840049, 1, 4, 116);
        CreateWall(3.994344, 3.108447, 33.42598, 1.615686, 1.604893, 1.623959, 4, 117);
        CreateWall(-5.272417, 5.538648, 25.75, 4.824834, 6.477295, 4, 4, 118);
        CreateWall(-5.272417, 5.538648, 36.227, 4.824834, 6.477295, 4, 4, 119);
        CreateWall(-6.90057, 6.730637, 46.3273, 11.26114, 8.861274, 20.97461, -1, 120);
        CreateWall(-7.831, 5.3, 30.99, 1, 6, 6.5, 4, 121);
        CreateWall(5.074815, 4.301765, 56.40557, 13.1601, 4.062151, 0.7663727, 4, 122);
        CreateWall(0.24, 7.568765, 47.13827, 3.5, 4.062151, 12.90346, 4, 123);
        CreateWall(-0.4394904, 4.314489, 43.90987, 1.612981, 4.03266, 3.21767, 4, 124);
        CreateWall(-11.73649, 7.54249, 64.4865, 1.612981, 4.03266, 4.060944, 0, 125);
        CreateWall(-11.92781, 7.54249, 56.60429, 1.995615, 4.03266, 2.005371, 0, 126);
        CreateWall(-14.16297, 7.145098, 55.19912, 3.225281, 3.237878, 3.219673, 0, 127);
        CreateWall(-13.34894, 7.145098, 52.7741, 1.610119, 3.237878, 1.614204, 0, 128);
        CreateWall(-11.92781, 7.54249, 62.85986, 1.995615, 4.03266, 0.8165054, 0, 129);
        CreateWall(-45.35478, 7.54249, 59.61, 1.089561, 4.03266, 15, 0, 130);
        CreateWall(-42.46, 5.924819, 39.06439, 5, 7.268002, 29.11122, 0, 131);
        CreateWall(-40.176, 5.523045, 55.21048, 0.4, 1.619911, 3.189045, 0, 132);
        CreateWall(-38.76395, 4.321394, 25.75254, 2.419907, 2.425211, 2.422911, 0, 133);
        CreateWall(-25.86095, 3.515394, 41.48711, 2.419907, 2.425211, 4.835773, 0, 134);
        CreateWall(-31.50165, 7.140826, 45.92187, 3.236599, 3.233137, 3.219515, 0, 135);
        CreateWall(-32.31604, 6.330677, 48.3438, 1.60783, 1.614568, 1.621651, 0, 136);
        CreateWall(-33.318, 4.250121, 50.35683, 0.4, 4.165757, 12.89635, 0, 137);
        CreateWall(-29.05922, 4.250121, 44.11132, 8.917554, 4.165757, 0.4053345, 0, 138);
        CreateWall(-24.24499, 6.744021, 40.68601, 0.8298302, 8.882464, 7.251068, 0, 139);
        CreateWall(-18.16362, 6.744021, 35.00214, 11.37258, 8.882464, 24.24732, 0, 140);
        CreateWall(-16.79598, 8.345907, 47.44, 8.42, 5.678691, 1, 0, 141);
        CreateWall(-25.05, 7.125232, 19.71861, 7.251068, 8.120045, 6.440567, 1, 142);
        CreateWall(-35.55, 7.125232, 19.71861, 7.251068, 8.120045, 6.440567, 1, 143);
        CreateWall(-37.39254, 7.125231, 23.74257, 5.316154, 8.120045, 1.612665, 0, 144);
        CreateWall(-20.57564, 3.187231, 15.68, 12.98743, 8.120045, 1.612665, 1, 145);
        CreateWall(-38.3665, 5.120971, 14.08765, 9.724064, 4.111524, 4.898634, 1, 146);
        CreateWall(-43.49247, 5.120971, 8.41167, 0.5960007, 4.111524, 6.454079, 1, 147);
        CreateWall(-41.58795, 5.120971, 1.959855, 3.235893, 4.111524, 12.92029, 1, 148);
        CreateWall(-34.34674, 4.740863, 0.3402195, 4.858311, 4.87174, 9.681019, 1, 149);
        CreateWall(-30.24487, 4.740863, 2.731868, 3.448055, 4.87174, 4.779274, 1, 150);
        CreateWall(-19.33138, 5.148415, 5.109592, 21.90529, 4.056636, 6.620091, 1, 151);
        CreateWall(-8.102446, 3.298522, 11.64577, 12.11511, 7.756417, 6.451536, -1, 152);
        CreateWall(-9.72221, 0.6884941, 7.204192, 2.422421, 2.419012, 2.422384, 1, 153);
        CreateWall(-17.38029, 0.2831234, 2.711911, 3.224581, 1.60827, 1.728946, 1, 154);
        CreateWall(-19.47366, 1.283146, 5.166797, 0.9559011, 3.613672, 6.495212, 1, 155);
        CreateWall(-19.01047, 1.429199, 10.02323, 3.239193, 4.145567, 3.21852, 1, 156);
        CreateWall(-25.45899, 5.416459, -2.911342, 6.440163, 6.200087, 6.407383, 2, 157);
        CreateWall(-34.32658, 3.935917, -5.298461, 1.617161, 3.239004, 1.625077, 2, 158);
        CreateWall(-42.02054, 5.132627, -21.06406, 0.8790817, 5.646835, 33.14563, 2, 159);
        CreateWall(-41.19003, 5.970626, -28.69818, 0.7959557, 5.646835, 7.276354, 2, 160);
        CreateWall(-39.97736, 3.099399, -20.8255, 3.224247, 1.623342, 0.3831959, 2, 161);
        CreateWall(-33.71442, 3.099399, -20.83454, 2.830128, 1.623342, 0.4012718, 2, 162);
        CreateWall(-33.92458, 3.504645, -19.42433, 2.409798, 2.433834, 2.412289, 2, 163);
        CreateWall(-32.10999, 3.099732, -25.9817, 0.4169121, 1.624008, 11.07064, 2, 164);
        CreateWall(-29.48116, 3.909713, -23.45477, 1.612955, 3.243968, 1.615158, 2, 165);
        CreateWall(-40.17027, 4.314253, -31.31842, 1.213169, 2.440887, 1.210466, 2, 166);
        CreateWall(-40.78803, 4.711897, -38.37749, 1.635225, 3.236175, 1.613446, 2, 167);
        CreateWall(-37.95386, 4.711897, -39.17149, 4.044044, 3.236175, 1.613446, 2, 168);
        CreateWall(-37.14289, 5.310734, -37.17551, 2.42997, 4.433849, 2.441495, 2, 169);
        CreateWall(-29.07345, 4.917582, -33.95314, 13.76087, 5.220153, 4.064755, 2, 170);
        CreateWall(-40.78566, 3.90744, -21.84602, 1.629536, 1.62726, 1.619009, 2, 171);
        CreateWall(15.65053, 3.52211, -1.713665, 4.895332, 2.395253, 5.782387, 3, 172);
        CreateWall(12.46407, 3.925133, 1.150944, 1.617191, 3.201298, 1.618112, 3, 173);
        CreateWall(5.992119, 5.469345, -0.4590559, 14.66269, 6.289724, 1.618112, 3, 174);
        CreateWall(23.35082, 3.525312, 3.975177, 2.418831, 2.389922, 2.420465, 3, 175);
        CreateWall(23.75113, 6.332403, -9.739486, 1.617455, 3.222512, 1.611141, 3, 176);
        CreateWall(21.54216, 3.514502, -2.880072, 0.4025955, 2.410711, 3.235695, 3, 177);
        CreateWall(-19.39825, 3.717201, -24.99569, 2.410248, 2.821984, 6.29, 2, 178);
        CreateWall(-17.58567, 3.108061, -24.66569, 1.205088, 1.603703, 5.62, 2, 179);
        CreateWall(-11.74238, 1.165568, -22.24077, 1.60723, 2.26469, 1.611542, 2, 180);
        CreateWall(-10.12338, 0.5635681, -23.04477, 1.60723, 2.26469, 1.611542, 2, 181);
        CreateWall(-10.12338, 0.371568, -21.43977, 1.60723, 2.26469, 1.611542, 2, 182);
        CreateWall(-24.23902, 2.723958, -26.694, 0.8139538, 0.8524289, 0.8340797, 2, 183);
        CreateWall(-17.78772, 2.671109, -21.45288, 0.8073368, 0.7361269, 0.775835, 2, 184);
        CreateWall(32.64186, 3.912763, 11.63979, 3.213718, 3.226475, 3.216652, 6, 185);
        CreateWall(35.05, 3.102, 10.828, 1.62, 1.62, 1.62, 5, 186);
        CreateWall(23.153, 3.307, -4.994, 2.82, 2, 1, 3, 187);
        CreateWall(-20.6154, 4.318905, 9.235, 3.226804, 1.613811, 1.6, 1, 188);
        CreateWall(8.033, 2.504, 0.538, 4.05, 0.4, 1.222159, 3, 189);
        CreateWall(18.112, 2.504, 28.781, 4.05, 0.4, 1.222159, 6, 190);
        CreateWall(15.9, 2.504, 43.898, 1.222159, 0.4, 4.05, 6, 191);
        CreateWall(-21.4125, 8.321119, -26.54848, 1.607979, 0.8150377, 4.404967, 2, 192);
        CreateWall(-25.052, 2.665476, -24.29729, 0.8, 0.7450484, 0.7311573, 2, 193);
        CreateWall(51.25, -0.15, 40.724, 1.77, 4.7418, 1.703053, 5, 194);
        CreateWall(43.89105, 0.4453565, 40.36826, 1.712103, 3.551087, 0.9915719, 5, 195);
        CreateWall(56.639, 4.118, 26.56685, 1.222159, 0.4, 4.037703, 5, 196);
        CreateWall(-20.6154, 6.148234, 9.235, 3.226804, 2.025973, 1.6, 1, 197);
        CreateWall(-21.006, 6.454825, 14.67238, 0.8, 1.41279, 0.415242, 1, 198);
        CreateWall(-21.006, 6.713649, 14.14657, 0.8, 0.895143, 0.8783064, 1, 199);
        CreateWall(-2.28019, 2.288647, 25.35139, 2.052381, 6.477295, 1.62215, 4, 200);
        CreateWall(-13.63971, 2.708109, -25.22188, 11.81, 2.43, 2.35, 2, 201);
    }
    else if (mapToLoad == TUTORIAL)
    {
        mapToSet = TUTORIAL;
        allMaps[mapToLoad].CollisionsCount = 8;
        allMaps[mapToLoad].AllWallsCollisions = malloc(allMaps[mapToLoad].CollisionsCount * sizeof(Wall));
        CreateWall(0, -1.5, 2, 22.7, 1, 22.7, 0, 0);
        CreateWall(-0.002059221, 0.227705, 1.943192, 2.450016, 2.451308, 2.462128, 0, 1);
        CreateWall(1.890654, -0.321328, 1.942942, 1.352347, 1.353242, 1.352881, 0, 2);
        CreateWall(0, -0.95, -6.720729, 22.7, 1, 5.33, 0, 3);
        CreateWall(-11.84, 3.77, 2, 1, 10, 22.7, 0, 4);
        CreateWall(11.84, 3.77, 2, 1, 10, 22.7, 0, 5);
        CreateWall(0, 3.77, -9.9, 22.7, 10, 1, 0, 6);
        CreateWall(0, 3.77, 13.785, 22.7, 10, 1, 0, 7);
    }
    else if (mapToLoad == DUST2_2x2)
    {
        mapToSet = DUST2_2x2;
        allMaps[mapToLoad].CollisionsCount = 104;
        allMaps[mapToLoad].AllWallsCollisions = malloc(allMaps[mapToLoad].CollisionsCount * sizeof(Wall));
        CreateWall(9.38846, -1.4, -8.007592, 35.77919, 1, 33.08558, 3, 0);
        CreateWall(3.991, 0.285, 4.786, 2.423, 2.423, 2.423, 3, 1);
        CreateWall(5.41, 1.107, 1.770854, 0.4, 4, 39.1, 3, 2);
        CreateWall(-1.4, 1.8, 38.985, 13, 1, 34.67, 4, 3);
        CreateWall(-1.654, 2.288648, 13.15623, 0.8, 6.477295, 26, 4, 4);
        CreateWall(24.42, 1.8, 29.52468, 37.5, 1, 67.90935, -1, 5);
        CreateWall(48.3451, 1.8, 1.665, 9.130207, 1, 38.67, 5, 6);
        CreateWall(40.63582, 1.8, -14.36, 5.668356, 1, 7, 5, 7);
        CreateWall(44.5, 5.04, -33.965, 15, 1, 6.67, 5, 8);
        CreateWall(47.75, -3.04, 37.732, 9, 1, 7, 5, 9);
        CreateWall(40.68, 4.23, -23.46, 5.7, 1, 11.2, 5, 10);
        CreateWall(27.58, 4.225, -24.69, 20, 1, 9, 3, 11);
        CreateWall(21.51, 4.225, -12.405, 7, 1, 15.17, 3, 12);
        CreateWall(-7.165, 5.03, 61.57, 11.17, 1, 10, 4, 13);
        CreateWall(55.00196, 3.415, 23.80224, 3.714117, 1, 17.83263, 5, 14);
        CreateWall(5.79, 1.107, -13.25915, 0.54, 3.71, 7.75, 3, 15);
        CreateWall(9.870321, 1.493199, -8.82, 8.488289, 4.839397, 1, 3, 16);
        CreateWall(19.71741, 1.488101, -10.13818, 11.26746, 4.849592, 1.627839, 3, 17);
        CreateWall(34.23705, 3.871293, -0.4494247, 19.34229, 9.615976, 20.9745, -1, 18);
        CreateWall(8.829233, 0.6898834, -10.13725, 1.602546, 3.232767, 1.615795, 3, 19);
        CreateWall(7.217544, 0.2866741, -16.19459, 2.415168, 2.426349, 2.424468, 3, 20);
        CreateWall(6.816122, 2.310524, -16.59494, 1.612324, 1.596437, 1.616609, 3, 21);
        CreateWall(18.91403, 1.488101, -23.54818, 11.28471, 4.849592, 1, 3, 22);
        CreateWall(9.246572, 1.488101, -20.25092, 8.058916, 4.849592, 5.70549, 3, 23);
        CreateWall(24.96741, 1.488101, -20.2333, 0.8, 4.849592, 5.66225, 3, 24);
        CreateWall(26.58551, 0.4672852, -18.67, 2.418206, 2.457632, 2.39, 3, 25);
        CreateWall(29.00651, 1.064285, -18.67, 2.418206, 2.457632, 2.39, 3, 26);
        CreateWall(26.98624, 2.499154, -19.01415, 1.618732, 1.627894, 1.629699, 3, 27);
        CreateWall(31.60709, 2.795956, -20.02006, 12.55215, 5.453778, 0.4001274, 3, 28);
        CreateWall(40.69431, 3.903672, -17.60006, 6.45, 3.238344, 0.4001274, 5, 29);
        CreateWall(37.69, 3.903672, -18.81139, 0.4001274, 3.238344, 2.822779, 5, 30);
        CreateWall(43.73696, 3.752151, -23.86887, 0.4140491, 3.541387, 12.91774, 5, 31);
        CreateWall(25.17, 4.305497, -15.58066, 0.4001274, 2.434694, 9.287472, 3, 32);
        CreateWall(38.68263, 5.510857, -19.60757, 1.611956, 1.60449, 3.610546, 5, 33);
        CreateWall(38.68263, 7.150856, -19.01406, 1.611956, 1.60449, 1.619928, 5, 34);
        CreateWall(42.71363, 5.530857, -22.65006, 1.611956, 1.60449, 1.619928, 5, 35);
        CreateWall(27.79171, 7.936794, -32.73777, 19.3618, 6.416366, 8.091341, 3, 36);
        CreateWall(27.78882, 7.936794, -28.30295, 7.260923, 6.416366, 0.8257637, 3, 37);
        CreateWall(15.6907, 7.936794, -13.78543, 4.817293, 6.416366, 29.87009, 3, 38);
        CreateWall(18.42, 7.936794, -20.63499, 1, 6.416366, 7.24859, 3, 39);
        CreateWall(44.73563, 3.656087, -19.84385, 1.611956, 2.038029, 1.555517, 5, 40);
        CreateWall(54.45702, 6.858232, -27.74011, 4.934025, 8.573491, 20.66022, 5, 41);
        CreateWall(44.79, 6.858232, -37.25, 15, 8.573491, 1, 5, 42);
        CreateWall(43.12514, 3.871293, -10.13021, 3.204861, 9.615976, 3.212919, 5, 43);
        CreateWall(43.12514, 3.871293, 9.217745, 3.204861, 9.615976, 3.228837, 5, 44);
        CreateWall(55.61611, 4.50092, -5.702986, 2.438217, 2.458159, 2.414028, 5, 45);
        CreateWall(55.15685, 5.471698, 5.165548, 6.338552, 6.415167, 19.35178, 5, 46);
        CreateWall(58.07824, 5.471698, 7.948711, 2.521341, 6.415167, 50.76546, 5, 47);
        CreateWall(56.02677, 4.719615, 15.67324, 1.615543, 1.62077, 1.627525, 5, 48);
        CreateWall(55.63088, 6.345802, 31.40095, 2.447762, 4.873144, 2.418945, 5, 49);
        CreateWall(47.67071, 6.720139, 36.50803, 13.9532, 8.840049, 7.817391, 5, 50);
        CreateWall(37.47268, 6.720139, 35.01363, 3.218556, 8.840049, 1.644658, 5, 51);
        CreateWall(38.30424, 6.720139, 36.66363, 4.881684, 8.840049, 1.644658, 5, 52);
        CreateWall(47.88821, -0.1699834, 41.53085, 8.275005, 4.7418, 1.703053, 5, 53);
        CreateWall(43.52215, 0.2628939, 31.01274, 0.8107128, 5.686295, 19.40301, 5, 54);
        CreateWall(44.72398, -1.73808, 39.10096, 1.634037, 1.618159, 3.295925, 5, 55);
        CreateWall(51.18998, -1.73808, 35.84166, 1.634037, 1.618159, 3.231308, 5, 56);
        CreateWall(51.18998, -0.1290795, 36.23938, 1.634037, 1.618159, 1.610428, 5, 57);
        CreateWall(52.32014, 0.8512607, 29.83927, 0.6802864, 6.955806, 20.27887, 5, 58);
        CreateWall(52.32679, 3.286222, 15.67575, 0.6669998, 2.08588, 1.603443, 5, 59);
        CreateWall(33.44603, 6.720139, 36.03182, 4.826495, 8.840049, 29.46829, 6, 60);
        CreateWall(22.95895, 6.720139, 21.99475, 3.240649, 8.840049, 24.45416, 6, 61);
        CreateWall(30.64663, 5.934331, 33.42, 0.9097431, 7.271662, 2.423, 6, 62);
        CreateWall(24.90663, 5.934331, 33.42, 0.9097431, 7.271662, 2.423, 6, 63);
        CreateWall(26.44053, 4.339704, 32.63532, 2.150795, 4.082407, 0.6293912, 6, 64);
        CreateWall(29.15415, 4.339704, 34.19246, 2.145238, 4.082407, 0.6865859, 6, 65);
        CreateWall(29.85332, 4.339704, 33.49086, 0.6353655, 4.082407, 0.3297293, 6, 66);
        CreateWall(25.68523, 4.339704, 33.37033, 0.6778869, 4.082407, 0.3050682, 6, 67);
        CreateWall(29.813, 3.51, 24.547, 2.423, 2.423, 2.423, 6, 68);
        CreateWall(24.90663, 5.934331, 22.13, 0.9097431, 7.271662, 2.423, 6, 69);
        CreateWall(30.64663, 5.934331, 22.13, 0.9097431, 7.271662, 2.423, 6, 70);
        CreateWall(26.37633, 6.720139, 57.01281, 10.12095, 8.840049, 13.31212, 6, 71);
        CreateWall(15.38498, 6.720139, 65.11506, 13.95091, 8.840049, 3.702241, 6, 72);
        CreateWall(-3.511163, 6.720139, 67.90365, 26.58, 8.840049, 2.839417, 0, 73);
        CreateWall(19.46931, 6.720139, 16.87856, 9.941291, 8.840049, 23.40178, -1, 74);
        CreateWall(11.42511, 6.720139, 16.45395, 6.092877, 8.840049, 22.55255, -1, 75);
        CreateWall(8.43699, 6.720139, 43.91084, 12.90911, 8.840049, 19.34007, -1, 76);
        CreateWall(15.2, 6.720139, 43.90626, 1, 8.840049, 7.272514, 6, 77);
        CreateWall(9.832866, 6.720139, 33.91, 10.07427, 8.840049, 1, 4, 78);
        CreateWall(3.994344, 3.108447, 33.42598, 1.615686, 1.604893, 1.623959, 4, 79);
        CreateWall(-5.272417, 5.538648, 25.75, 4.824834, 6.477295, 4, 4, 80);
        CreateWall(-5.272417, 5.538648, 36.227, 4.824834, 6.477295, 4, 4, 81);
        CreateWall(-6.90057, 6.730637, 46.3273, 11.26114, 8.861274, 20.97461, -1, 82);
        CreateWall(-7.831, 5.3, 30.99, 1, 6, 6.5, 4, 83);
        CreateWall(5.074815, 4.301765, 56.40557, 13.1601, 4.062151, 0.7663727, 4, 84);
        CreateWall(0.24, 7.568765, 47.13827, 3.5, 4.062151, 12.90346, 4, 85);
        CreateWall(-0.4394904, 4.314489, 43.90987, 1.612981, 4.03266, 3.21767, 4, 86);
        CreateWall(-11.73649, 7.54249, 61.2065, 1.612981, 4.03266, 12.21, 0, 87);
        CreateWall(15.65053, 3.52211, -1.713665, 4.895332, 2.395253, 5.782387, 3, 88);
        CreateWall(12.46407, 3.925133, 1.150944, 1.617191, 3.201298, 1.618112, 3, 89);
        CreateWall(5.992119, 3.914345, -0.4590559, 14.66269, 10.87, 1.618112, 3, 90);
        CreateWall(23.35082, 3.525312, 3.975177, 2.418831, 2.389922, 2.420465, 3, 91);
        CreateWall(23.75113, 6.332403, -9.739486, 1.617455, 3.222512, 1.611141, 3, 92);
        CreateWall(21.54216, 3.514502, -2.880072, 0.4025955, 2.410711, 3.235695, 3, 93);
        CreateWall(32.64186, 3.912763, 11.63979, 3.213718, 3.226475, 3.216652, 6, 94);
        CreateWall(35.05, 3.102, 10.828, 1.62, 1.62, 1.62, 5, 95);
        CreateWall(23.153, 3.307, -4.994, 2.82, 2, 1, 3, 96);
        CreateWall(8.033, 2.504, 0.538, 4.05, 0.4, 1.222159, 3, 97);
        CreateWall(18.112, 2.504, 28.781, 4.05, 0.4, 1.222159, 6, 98);
        CreateWall(15.9, 2.504, 43.898, 1.222159, 0.4, 4.05, 6, 99);
        CreateWall(51.25, -0.15, 40.724, 1.77, 4.7418, 1.703053, 5, 100);
        CreateWall(43.89105, 0.4453565, 40.36826, 1.712103, 3.551087, 0.9915719, 5, 101);
        CreateWall(56.639, 4.118, 26.56685, 1.222159, 0.4, 4.037703, 5, 102);
        CreateWall(-2.28019, 2.288647, 25.35139, 2.052381, 6.477295, 1.62215, 4, 103);
    }
    else if (mapToLoad == AIM_MAP)
    {
        mapToSet = AIM_MAP;
        allMaps[mapToLoad].CollisionsCount = 87;
        allMaps[mapToLoad].AllWallsCollisions = malloc(allMaps[mapToLoad].CollisionsCount * sizeof(Wall));
        CreateWall(0, -1.5, 0.3719978, 35.8, 1, 60, 0, 0);
        CreateWall(-16.44, 3.77, 0.3719978, 1, 10, 60, 0, 1);
        CreateWall(16.55, 3.77, 0.3719978, 1, 10, 60, 0, 2);
        CreateWall(0, 3.77, -29.248, 33, 10, 1, 0, 3);
        CreateWall(0, 3.77, 29.592, 33, 10, 1, 0, 4);
        CreateWall(-1.4, 1.24, 23.492, 30.5, 5.5, 0.5, 0, 5);
        CreateWall(-1.4, 0.35, 26.372, 30.5, 5.5, 6, 0, 6);
        CreateWall(1.44, 1.24, -23.128, 30.5, 5.5, 0.5, 0, 7);
        CreateWall(1.44, 0.35, -26.188, 30.5, 5.5, 6, 0, 8);
        CreateWall(-14.89, 0.35, -26.353, 2.6, 5.5, 6.17, 0, 9);
        CreateWall(-13.61, -0.497, -20.141, 0.4, 1, 6.51, 0, 10);
        CreateWall(-13.61, 0.53, -21.564, 0.4, 1, 6.51, 0, 11);
        CreateWall(-13.61, 1.56, -23.065, 0.4, 1, 6.51, 0, 12);
        CreateWall(-13.61, 2.616, -22.264, 0.4, 1, 1.74, 0, 13);
        CreateWall(14.99, 0.35, 26.537, 2.76, 5.5, 5.87, 0, 14);
        CreateWall(13.66, -0.54, 20.471, 0.4, 1, 6.51, 0, 15);
        CreateWall(13.66, 0.51, 21.93, 0.4, 1, 6.51, 0, 16);
        CreateWall(13.66, 1.55, 23.46, 0.4, 1, 6.51, 0, 17);
        CreateWall(13.66, 2.597, 22.62, 0.4, 1, 1.77, 0, 18);
        CreateWall(7.937, 3.985, 24.576, 1.8, 1.8, 1.8, 0, 19);
        CreateWall(-9.906, 3.985, 24.576, 1.8, 1.8, 1.8, 0, 20);
        CreateWall(9.952, 3.985, -24.247, 1.8, 1.8, 1.8, 0, 21);
        CreateWall(-7.897, 3.985, -24.245, 1.8, 1.8, 1.8, 0, 22);
        CreateWall(-10.224, -0.042, -14.01, 1.8, 1.9, 1.8, 0, 23);
        CreateWall(-5.55, -0.042, -12.903, 1.8, 1.9, 1.8, 0, 24);
        CreateWall(4.21, -0.13, -13.924, 1.8, 1.8, 1.8, 0, 25);
        CreateWall(2.066, 0.24, -12.954, 2.54, 2.44, 2.14, 0, 26);
        CreateWall(-3.735, 0.826, -11.866, 1.81, 3.56, 1.8, 0, 27);
        CreateWall(-11.989, 0.826, -13.02, 1.81, 3.56, 1.8, 0, 28);
        CreateWall(-2.43, -0.371, -13.31, 1.38, 1.37, 1.15, 0, 29);
        CreateWall(-8.898, -0.5, -14.659, 0.86, 1, 0.9, 0, 30);
        CreateWall(3.79, 0.826, 12.198, 1.81, 3.56, 1.8, 0, 31);
        CreateWall(12.04, 0.826, 13.337, 1.81, 3.56, 1.8, 0, 32);
        CreateWall(-2.008, 0.24, 13.278, 2.54, 2.44, 2.14, 0, 33);
        CreateWall(-4.159, -0.184, 14.257, 1.8, 1.9, 1.8, 0, 34);
        CreateWall(5.58, -0.042, 13.229, 1.8, 1.9, 1.8, 0, 35);
        CreateWall(10.264, -0.042, 14.343, 1.8, 1.9, 1.8, 0, 36);
        CreateWall(8.96, -0.521, 14.982, 0.86, 1, 0.9, 0, 37);
        CreateWall(2.469, -0.343, 13.647, 1.33, 1.32, 1.13, 0, 38);
        CreateWall(16.24, 3.429, 2.05, 2.87, 8.82, 0.9, 0, 39);
        CreateWall(16.22, 3.429, 2.76, 1.52, 8.82, 0.9, 0, 40);
        CreateWall(16.24, 3.429, -2.42, 2.87, 8.82, 0.9, 0, 41);
        CreateWall(16.22, 3.429, -3.13, 1.52, 8.82, 0.9, 0, 42);
        CreateWall(16.08, 3.429, -0.17, 1.52, 8.82, 3.8, 0, 43);
        CreateWall(16.3, 5.28, -0.15, 2.87, 5.62, 3.61, 0, 44);
        CreateWall(-16.29, 3.429, 2.031, 2.87, 8.82, 0.9, 0, 45);
        CreateWall(-16.29, 3.429, -2.44, 2.87, 8.82, 0.9, 0, 46);
        CreateWall(-16.28, 3.429, 2.741, 1.52, 8.82, 0.9, 0, 47);
        CreateWall(-16.28, 3.429, -3.149, 1.52, 8.82, 0.9, 0, 48);
        CreateWall(-16.06, 3.429, -0.13, 1.52, 8.82, 3.7, 0, 49);
        CreateWall(-16.29, 5.28, -0.21, 2.87, 5.62, 3.61, 0, 50);
        CreateWall(16.45, 4.85, 9.15, 1.4, 5.62, 4.1, 0, 51);
        CreateWall(16.45, 4.85, -9.420001, 1.4, 5.62, 4.1, 0, 52);
        CreateWall(-16.38, 4.85, 9.09, 1.4, 5.62, 4.1, 0, 53);
        CreateWall(-16.38, 4.85, -9.48, 1.4, 5.62, 4.1, 0, 54);
        CreateWall(-10.725, -0.872, 20.25, 10.98, 1.1, 5.96, 0, 55);
        CreateWall(-12.68, -0.872, 15, 10.33, 1.1, 5.6, 0, 56);
        CreateWall(-13.58, -0.872, 10.41, 6.48, 1.1, 4.29, 0, 57);
        CreateWall(-5.109, 0.01300001, 22.286, 0.5, 1.1, 2.03, 0, 58);
        CreateWall(-7.56, 0.01300001, 14.786, 0.5, 1.1, 5.29, 0, 59);
        CreateWall(-10.47, 0.01300001, 10.334, 0.5, 1.1, 4.46, 0, 60);
        CreateWall(-6.096, 0.01300001, 17.25, 2.31, 1.1, 0.5, 0, 61);
        CreateWall(-9.027, 0.01300001, 12.339, 2.29, 1.1, 0.5, 0, 62);
        CreateWall(-13.81, 0.01300001, 8.322, 6.04, 1.1, 0.5, 0, 63);
        CreateWall(10.795, -0.872, -19.86, 10.98, 1.1, 5.96, 0, 64);
        CreateWall(11.89, -0.872, -14.51, 8.6, 1.1, 5.13, 0, 65);
        CreateWall(13.56, -0.872, -10.15, 6.15, 1.1, 4.3, 0, 66);
        CreateWall(5.149, 0.01300001, -22.004, 0.5, 1.1, 2.17, 0, 67);
        CreateWall(7.606, 0.01300001, -14.453, 0.5, 1.1, 5.29, 0, 68);
        CreateWall(10.502, 0.01300001, -10.006, 0.5, 1.1, 4.42, 0, 69);
        CreateWall(13.475, 0.01300001, -7.994, 5.41, 1.1, 0.5, 0, 70);
        CreateWall(9.036, 0.01300001, -12.009, 2.24, 1.1, 0.5, 0, 71);
        CreateWall(6.073, 0.01300001, -16.913, 2.4, 1.1, 0.5, 0, 72);
        CreateWall(7.111, 0.353, -17.671, 1.39, 1.31, 1.14, 0, 73);
        CreateWall(8.674, 0.544, -13.088, 1.79, 1.82, 1.81, 0, 74);
        CreateWall(11.584, 0.544, -9.076, 1.79, 1.82, 1.81, 0, 75);
        CreateWall(15.08, 0.811, -19.223, 1.99, 2.18, 2.01, 0, 76);
        CreateWall(12.373, 0.436, -22.116, 1.59, 1.6, 1.57, 0, 77);
        CreateWall(12.97, 0.175, -8.692, 0.99, 1.01, 1, 0, 78);
        CreateWall(14.68, 1.42, -21.56, 3.03, 3.64, 2.66, 0, 79);
        CreateWall(-14.72, 1.42, 21.96, 3.03, 3.64, 2.66, 0, 80);
        CreateWall(-13.027, 0.175, 9.08, 0.99, 1.01, 1, 0, 81);
        CreateWall(-7.153, 0.353, 18.049, 1.39, 1.31, 1.14, 0, 82);
        CreateWall(-8.728, 0.544, 13.485, 1.79, 1.82, 1.81, 0, 83);
        CreateWall(-11.626, 0.544, 9.467, 1.79, 1.82, 1.81, 0, 84);
        CreateWall(-15.167, 0.8099999, 19.62, 2.09, 2.2, 2.03, 0, 85);
        CreateWall(-12.415, 0.438, 22.515, 1.58, 1.6, 1.59, 0, 86);
    }
    else if (mapToLoad == B2000)
    {
        mapToSet = B2000;
        allMaps[mapToLoad].CollisionsCount = 42;
        allMaps[mapToLoad].AllWallsCollisions = malloc(allMaps[mapToLoad].CollisionsCount * sizeof(Wall));
        CreateWall(-0.1639996, -1.6, 0.3489971, 44, 1, 50, 0, 0);
        CreateWall(-0.1639996, 0.76, -21.98, 42.6, 3.16, 2, 0, 1);
        CreateWall(-0.1639996, 3.94, -23.81, 43, 3.16, 2, 0, 2);
        CreateWall(-0.1639996, 0.76, 22.2, 42.6, 3.16, 2, 0, 3);
        CreateWall(-0.1639996, 3.94, 23.99, 43, 3.16, 2, 0, 4);
        CreateWall(-20.82, 3.1, 0.1269993, 1, 8, 47, 0, 5);
        CreateWall(20.63, 3.1, 0.1269993, 1, 8, 47, 0, 6);
        CreateWall(-0.1639996, 7.41, -23.013, 43, 3.16, 2, 0, 7);
        CreateWall(-0.1639996, 7.41, 23.217, 43, 3.16, 2, 0, 8);
        CreateWall(-17.88, 0.5779999, 13.99, 5.2, 3, 1, 0, 9);
        CreateWall(-17.88, 0.5779999, 4.34, 5.2, 3, 1, 0, 10);
        CreateWall(-17.88, 0.5779999, -4.13, 5.2, 3, 1, 0, 11);
        CreateWall(-17.88, 0.5779999, -13.8, 5.2, 3, 1, 0, 12);
        CreateWall(17.7, 0.5779999, -13.8, 5.2, 3, 1, 0, 13);
        CreateWall(17.7, 0.5779999, -4.12, 5.2, 3, 1, 0, 14);
        CreateWall(17.7, 0.5779999, 4.34, 5.2, 3, 1, 0, 15);
        CreateWall(17.7, 0.5779999, 13.99, 5.2, 3, 1, 0, 16);
        CreateWall(-0.08, 3.04, 8.58, 20.35, 8, 5.08, 0, 17);
        CreateWall(-0.1, 3.04, -8.39, 20.35, 8, 5.08, 0, 18);
        CreateWall(-6.4, 3.04, 0.105, 7.8, 8, 5.1, 0, 19);
        CreateWall(6.32, 3.04, 0.105, 7.6, 8, 5.1, 0, 20);
        CreateWall(-0.1639996, 7.79, 0.3489971, 40, 1, 45.5, 0, 21);
        CreateWall(-19.474, 0.76, 20.512, 1.87, 3.16, 1.6, 0, 22);
        CreateWall(19.329, 0.76, 20.51, 1.87, 3.16, 1.6, 0, 23);
        CreateWall(19.329, 0.76, -20.21, 1.87, 3.16, 1.68, 0, 24);
        CreateWall(-19.476, 0.76, -20.21, 1.87, 3.16, 1.68, 0, 25);
        CreateWall(-20.009, 2.89, -13.773, 0.9, 0.5, 0.65, 0, 26);
        CreateWall(19.841, 2.886, -13.787, 0.9, 0.5, 0.65, 0, 27);
        CreateWall(-20.035, 2.89, 13.977, 0.9, 0.5, 0.65, 0, 28);
        CreateWall(19.841, 2.886, 13.977, 0.9, 0.5, 0.65, 0, 29);
        CreateWall(-16.4, -0.49, -20.21, 3.5, 1, 1.68, 0, 30);
        CreateWall(-17.35, 0.521, -20.21, 1.7, 1, 1.68, 0, 31);
        CreateWall(-18.158, 1.539, -20.21, 0.2, 1, 1.68, 0, 32);
        CreateWall(-16.53, -0.49, 20.544, 3.5, 1, 1.68, 0, 33);
        CreateWall(-17.31, 0.521, 20.544, 1.7, 1, 1.68, 0, 34);
        CreateWall(-18.108, 1.539, 20.544, 0.2, 1, 1.68, 0, 35);
        CreateWall(16.36, -0.49, 20.544, 3.5, 1, 1.68, 0, 36);
        CreateWall(17.17, 0.521, 20.544, 1.7, 1, 1.68, 0, 37);
        CreateWall(17.966, 1.539, 20.544, 0.2, 1, 1.68, 0, 38);
        CreateWall(16.26, -0.49, -20.21, 3.5, 1, 1.68, 0, 39);
        CreateWall(17.04, 0.521, -20.21, 1.7, 1, 1.68, 0, 40);
        CreateWall(17.979, 1.539, -20.21, 0.2, 1, 1.68, 0, 41);
    }
    else if (mapToLoad == MIRAGEA)
    {
        mapToSet = MIRAGEA;
        allMaps[mapToLoad].CollisionsCount = 97;
        allMaps[mapToLoad].AllWallsCollisions = malloc(allMaps[mapToLoad].CollisionsCount * sizeof(Wall));
        CreateWall(-17.82, -1.47, -31.56, 57.7, 1, 30.9, 4, 0);
        CreateWall(17.065, 0.98, 40.86, 14.37, 1, 4.7, 0, 1);
        CreateWall(19.19, 3.57, 43.22, 1.3, 9, 20.3, 0, 2);
        CreateWall(12.78, 4.03, 31.67, 36.8, 10, 14.58, 0, 3);
        CreateWall(16.8, 6.54, 49.281, 14, 10, 1, 0, 4);
        CreateWall(-4.15, 6.75, 47.76, 29.8, 10, 3, 0, 5);
        CreateWall(-8, -1.47, 31.2, 38, 1, 25, 0, 6);
        CreateWall(1.73, 0.98, 46.17, 39.83, 1, 6, 0, 7);
        CreateWall(-11.3, 3.4, -36.69, 25.63, 8.35, 8.32, 4, 8);
        CreateWall(-39.89, 3.4, -36.91, 15.38, 8.35, 8.72, 4, 9);
        CreateWall(-28.1, 3.4, -41.77, 11.7, 8.35, 1, 4, 10);
        CreateWall(-3.23, -0.4, 43.61, 24.14, 1, 1, 0, 11);
        CreateWall(-9.7, 5.25, 12.93, 11.5, 7.5, 23, 3, 12);
        CreateWall(-4.72, 1.8, 43.31, 14.8, 0.6, 0.2, 0, 13);
        CreateWall(-0.65, 0.98, -21.91, 20.54, 1, 9.3, 4, 14);
        CreateWall(-28.59, 0.98, 0.21, 32.47, 1, 33, 3, 15);
        CreateWall(-14.87, 0.98, -16.93, 10.46, 1, 11.8, 4, 16);
        CreateWall(-40.38, 0.98, -14.825, 8.2, 1, 3.12, 3, 17);
        CreateWall(-21.15, 0.98, 15.125, 6.5, 1, 3.12, 3, 18);
        CreateWall(-16.89, 0.98, 20.15, 2, 1, 8, 0, 19);
        CreateWall(-24.51, 3.75, 24.38, 1, 9.5, 17.16, 0, 20);
        CreateWall(-15.35, 2.44, 20.14, 1, 6.71, 7.9, 0, 21);
        CreateWall(-22.345, 3.75, 16.612, 3.16, 4.4, 0.82, 3, 22);
        CreateWall(-26.09, 3.95, 14.03, 4.1, 5.23, 2.6, 3, 23);
        CreateWall(-31.09, 5.22, 15.79, 10.04, 7.2, 0.82, 3, 24);
        CreateWall(-39.49, 5.22, 10.11, 8, 7.2, 3, 3, 25);
        CreateWall(-13.4, 3.69, 38.16, 2.46, 9, 10.5, 0, 26);
        CreateWall(-20.82, 3.69, 33.19, 17, 9, 1.65, 0, 27);
        CreateWall(-13.78, 3.69, 24.59, 11.5, 9, 0.85, 0, 28);
        CreateWall(-7.09, 3.69, 26.93, 2, 9, 3.5, 0, 29);
        CreateWall(-11.19, 0.8200001, 35.65, 1.9, 3.5, 10.34, 0, 30);
        CreateWall(-17.5, 0.8200001, 31.41, 14.02, 3.5, 1.9, 0, 31);
        CreateWall(-11.3, -0.325, 42.324, 1.7, 1.7, 1.7, 0, 32);
        CreateWall(-11.714, 1.215, 42.212, 0.9, 0.9, 0.9, 0, 33);
        CreateWall(-8.915, 0.691, 26.53, 1.6, 3.2, 3.5, 0, 34);
        CreateWall(-19.96, 0.98, 38.72, 11.28, 1, 9.8, 0, 35);
        CreateWall(-30.115, 3.436, 36.26, 10.67, 1, 8, 0, 36);
        CreateWall(-38.445, 4.51, 35.66, 7.67, 0.5, 6.5, 0, 37);
        CreateWall(-31.81, 4.51, 24.385, 4.9, 0.5, 13.67, 0, 38);
        CreateWall(-37.69, 4.51, 21.85, 6.83, 0.5, 22, 0, 39);
        CreateWall(-32.78, 4.61, 39.3, 17, 9, 1, 0, 40);
        CreateWall(-21.18, 4.61, 41.99, 6.61, 7, 9.5, 0, 41);
        CreateWall(-41.29, 6.26, 25.11, 1, 4, 28, 0, 42);
        CreateWall(-28.87, 6.26, 25.41, 1, 4, 17, 0, 43);
        CreateWall(-33.468, 6.29, 17.14, 7.94, 3, 1, 0, 44);
        CreateWall(-35.086, 6.29, 21.727, 1.65, 3, 1.65, 0, 45);
        CreateWall(-35.086, 6.29, 26.641, 1.65, 3, 1.65, 0, 46);
        CreateWall(-35.086, 6.29, 31.549, 1.65, 3, 1.65, 0, 47);
        CreateWall(-39.982, 5.576, 26.847, 1.64, 1.64, 1.64, 0, 48);
        CreateWall(-12.594, 3.061, 43.366, 0.8, 3, 0.5, 0, 49);
        CreateWall(-35.689, 6.44, 15.55, 3.7, 3.2, 3.62, 0, 50);
        CreateWall(-33.26, 4.51, 12.58, 4.51, 0.5, 5.42, 3, 51);
        CreateWall(-28.738, 4.51, 12.58, 1.21, 0.5, 5.42, 3, 52);
        CreateWall(-30.179, 4.51, 12.008, 1.7, 0.5, 4.3, 3, 53);
        CreateWall(-30.897, 4.87, 9.959, 5.48, 1, 0.2, 3, 54);
        CreateWall(-27.318, 3.156, 11.701, 1.65, 3.2, 2, 3, 55);
        CreateWall(-25.664, 2.116, 11.861, 1.63, 1.63, 1.63, 3, 56);
        CreateWall(-16.262, 2.1, 6.187, 1.63, 1.63, 1.63, 3, 57);
        CreateWall(-16.628, 3.585, 5.821, 0.9, 0.9, 0.9, 3, 58);
        CreateWall(-18.72, 3.203, 6.179, 3.3, 3.1, 1.65, 3, 59);
        CreateWall(-36.17, 2.169, 7.956, 1.4, 1.4, 1.4, 3, 60);
        CreateWall(-34.356, 2.611, 8.981998, 2.3, 2.3, 2.3, 3, 61);
        CreateWall(-38.774, 2.299, 4.519, 1.65, 1.65, 1.65, 3, 62);
        CreateWall(-40.402, 2.299, 3.237, 1.65, 1.65, 1.65, 3, 63);
        CreateWall(-33.245, 2.299, 2.704, 1.65, 1.65, 1.65, 3, 64);
        CreateWall(-32.642, 2.299, -3.541, 1.65, 1.65, 1.65, 3, 65);
        CreateWall(-33.642, 2.299, -5.189, 1.65, 1.65, 1.65, 3, 66);
        CreateWall(-33.099, 3.955, -4.446, 1.65, 1.65, 1.65, 3, 67);
        CreateWall(-32.842, 3.13, 4.333, 1.65, 3.3, 1.65, 3, 68);
        CreateWall(-40.591, 2.83, -9.155001, 1.3, 2.56, 1.3, 3, 69);
        CreateWall(-42.422, 2.117, -11.013, 1.65, 1.65, 1.65, 4, 70);
        CreateWall(-36.725, 2.299, -11.007, 1.65, 1.65, 1.65, 3, 71);
        CreateWall(-43.217, 4.99, -0.71, 4, 7, 19.01, 3, 72);
        CreateWall(-27.72, 5.78, -17.95, 17.2, 13.7, 12.3, 4, 73);
        CreateWall(-27.72, 4.61, -11.16, 16.5, 6.1, 3.6, 3, 74);
        CreateWall(-45.46, 3.95, -20.26, 2, 10, 28, 4, 75);
        CreateWall(-15.56, 4.61, 2.08, 5.55, 6.1, 6.6, 3, 76);
        CreateWall(-26.026, 3.588, 12.247, 0.9, 0.9, 0.9, 3, 77);
        CreateWall(-19.94, 3.12, -0.5700001, 3.3, 3.3, 3.3, 3, 78);
        CreateWall(-19.96, 4.828, 22.224, 8.24, 1, 3.3, 0, 79);
        CreateWall(-16.674, 2.609, -1.503, 2, 2.1, 0.6, 3, 80);
        CreateWall(-14.214, 2.069, -1.503, 2, 1, 0.6, 3, 81);
        CreateWall(-11.74, 4.99, -8.6, 1.72, 7, 18.8, 3, 82);
        CreateWall(-13.44, 4.99, -12.846, 2, 7, 2.1, 3, 83);
        CreateWall(-15.5, 4.03, -24.29, 10.8, 9.5, 4.6, 4, 84);
        CreateWall(-28.73, 1.56, -25.04, 11.1, 5, 1.5, 4, 85);
        CreateWall(-7.81, 3.4, -23.49, 5.2, 8.35, 1.2, 4, 86);
        CreateWall(-0.986, 3.4, -23.49, 5.2, 8.35, 1.2, 4, 87);
        CreateWall(-4.387, 7.54, -23.49, 1.5, 8.35, 1.2, 4, 88);
        CreateWall(5.11, 5.04, -24.91, 7.66, 8.35, 17.99, 4, 89);
        CreateWall(2.98, 3.96, -22.05, 12.33, 4, 1.65, 4, 90);
        CreateWall(-0.1, 2.299, -20.42, 3.3, 1.65, 1.65, 4, 91);
        CreateWall(-18.312, 2.313, -18.785, 1.65, 1.65, 3.3, 4, 92);
        CreateWall(0.575, 0.98, -29.605, 2.67, 1, 6.1, 4, 93);
        CreateWall(-0.87, 3.56, -16.362, 19.53, 7.43, 1.65, 4, 94);
        CreateWall(-10.549, 4.74, -18.017, 0.9, 7.43, 1.65, 4, 95);
        CreateWall(-10.549, 4.74, -22.095, 0.9, 7.43, 1.65, 4, 96);
    }
}

/**
 * @brief Add stairs for dust 2 map
 * Generated with the Unity project
 */
void LoadAllStairs(int mapToLoad)
{
    if (mapToLoad == DUST2)
    {
        mapToSet = DUST2;
        allMaps[mapToLoad].StairsCount = 31;
        allMaps[mapToLoad].AllStairs = malloc(allMaps[mapToLoad].StairsCount * sizeof(Stairs));
        CreateStairs(-1.257, 5.2, 8.419, 21.325, 0, 3.211, 2, 0);
        CreateStairs(-1.252, -0.85, 3.576, 6.8, 0, 0.388, 1, 1);
        CreateStairs(5.3, 6.45, 21.313, 56.32, 3.24, 3.24, 1, 2);
        CreateStairs(41.98, 45.06, 10.7, 21.311, 3.24, 3.24, 2, 3);
        CreateStairs(24.558, 37.4724, -20.03, -10.9489, 0, 3.2112, 3, 4);
        CreateStairs(42.87, 44.44, -17.4, -11.755, 3.24, 3.24, 2, 5);
        CreateStairs(50.37, 56.8304, -17.4011, -4.494, 3.237, 4.8242, 3, 6);
        CreateStairs(43.93, 51.99, -30.31, -17.403, 3.228, 6.451, 0, 7);
        CreateStairs(43.93, 51.99, 21.315, 34.231, -1.613, 3.217, 0, 8);
        CreateStairs(49.58, 52.804, 16.485, 19.704, 3.228, 4.829, 3, 9);
        CreateStairs(18.111, 21.33, -4.495, 0.33, 3.237, 5.64, 0, 10);
        CreateStairs(37.48, 43.51, -30.31, -28.293, 5.647, 6.46, 0, 11);
        CreateStairs(37.13, 38.37, -28.69, -20.22, 5.632, 5.632, 2, 12);
        CreateStairs(18.89, 24.98, -20.98, -19.1, 5.632, 5.632, 3, 13);
        CreateStairs(-17.385, -7.699999, -26.87, -10.9489, 0, 3.211, 1, 14);
        CreateStairs(-38.35, -35.131, -20.628, -19.037, 3.237, 4.026, 0, 15);
        CreateStairs(-39.974, -36.749, -6.12, -4.495998, 3.237, 4.026, 2, 16);
        CreateStairs(-34.28, -33.04, 4.909999, 13.38, 4.02, 4.02, 2, 17);
        CreateStairs(-32.48, -28.09, 15.51, 17.13, 4.02, 4.02, 3, 18);
        CreateStairs(-39.972, -23.841, 26.966, 28.46, 3.228, 4.019, 0, 19);
        CreateStairs(-39.97, -33.52, 43.904, 56.818, 3.23, 6.449, 2, 20);
        CreateStairs(-1.248, 11.659, 56.79, 66.49001, 3.227, 6.441, 1, 21);
        CreateStairs(-33.96, -32.95, 56.824, 63.264, 6.44, 6.44, 0, 22);
        CreateStairs(-40.67, -39.49, 56.824, 63.264, 6.44, 6.44, 0, 23);
        CreateStairs(-13.74, -12.56, 56.824, 63.264, 6.44, 6.44, 0, 24);
        CreateStairs(-44.813, -12.546, 63.2688, 66.496, 6.454, 7.255, 2, 25);
        CreateStairs(-17.383, -14.166, 8.419, 11.642, 0.4100001, 2.006, 2, 26);
        CreateStairs(-20.3, -17.214, 11.644, 14.868, 2.073, 4.018, 1, 27);
        CreateStairs(-20.585, -18.3, -25.084, -23.196, 6.058, 6.893, 0, 28);
        CreateStairs(-22.7, -22.23, -27.867, -26.293, 6.457, 6.843, 3, 29);
        CreateStairs(-23.845, -22.226, -26.263, -25.927, 6.052, 6.439, 0, 30);
    }
    else if (mapToLoad == TUTORIAL)
    {
        mapToSet = TUTORIAL;
        allMaps[mapToLoad].StairsCount = 1;
        allMaps[mapToLoad].AllStairs = malloc(allMaps[mapToLoad].StairsCount * sizeof(Stairs));
        CreateStairs(-11.344, 11.336, -3.7246, -3.15, -0.071, 0.471, 0, 0);
    }
    else if (mapToLoad == DUST2_2x2)
    {
        mapToSet = DUST2_2x2;
        allMaps[mapToLoad].StairsCount = 14;
        allMaps[mapToLoad].AllStairs = malloc(allMaps[mapToLoad].StairsCount * sizeof(Stairs));
        CreateStairs(-1.257, 5.2, 8.419, 21.325, 0, 3.211, 2, 0);
        CreateStairs(5.3, 6.45, 21.313, 56.32, 3.24, 3.24, 1, 1);
        CreateStairs(41.98, 45.06, 10.7, 21.311, 3.24, 3.24, 2, 2);
        CreateStairs(24.558, 37.4724, -20.03, -10.9489, 0, 3.2112, 3, 3);
        CreateStairs(42.87, 44.44, -17.4, -11.755, 3.24, 3.24, 2, 4);
        CreateStairs(50.37, 56.8304, -17.4011, -4.494, 3.237, 4.8242, 3, 5);
        CreateStairs(43.93, 51.99, -30.31, -17.403, 3.228, 6.451, 0, 6);
        CreateStairs(43.93, 51.99, 21.315, 34.231, -1.613, 3.217, 0, 7);
        CreateStairs(49.58, 52.804, 16.485, 19.704, 3.228, 4.829, 3, 8);
        CreateStairs(18.111, 21.33, -4.495, 0.33, 3.237, 5.64, 0, 9);
        CreateStairs(37.48, 43.51, -30.31, -28.293, 5.647, 6.46, 0, 10);
        CreateStairs(37.13, 38.37, -28.69, -20.22, 5.632, 5.632, 2, 11);
        CreateStairs(18.89, 24.98, -20.98, -19.1, 5.632, 5.632, 3, 12);
        CreateStairs(-1.248, 11.659, 56.79, 66.49001, 3.227, 6.441, 1, 13);
    }
    else if (mapToLoad == AIM_MAP)
    {
        mapToSet = AIM_MAP;
        allMaps[mapToLoad].StairsCount = 6;
        allMaps[mapToLoad].AllStairs = malloc(allMaps[mapToLoad].StairsCount * sizeof(Stairs));
        CreateStairs(-16.2, -13.593, -22.929, -16.86, -0.071, 4.013, 0, 0);
        CreateStairs(13.602, 16.37, 17.217, 23.266, -0.071, 4.01, 2, 1);
        CreateStairs(-4.9, -3.949999, 17.273, 23.23, -0.073, 0.5920001, 1, 2);
        CreateStairs(3.78, 4.97, -22.845, -16.883, -0.061, 0.5910001, 3, 3);
        CreateStairs(-15.324, -14.037, -1.96, 1.600002, -0.073, 0.358, 0, 4);
        CreateStairs(13.981, 15.23, -1.940001, 1.6, -0.06, 0.3430001, 2, 5);
    }
    else if (mapToLoad == B2000)
    {
        mapToSet = B2000;
        allMaps[mapToLoad].StairsCount = 4;
        allMaps[mapToLoad].AllStairs = malloc(allMaps[mapToLoad].StairsCount * sizeof(Stairs));
        CreateStairs(-18.233, -12.459, 19.726, 21.343, -0.073, 3.303, 1, 0);
        CreateStairs(12.289, 18.039, -21.078, -19.386, -0.07300002, 3.281, 3, 1);
        CreateStairs(-18.23, -12.492, -21.078, -19.386, -0.073, 3.277, 1, 2);
        CreateStairs(12.25, 18.043, 19.73, 21.34, -0.07300002, 3.298, 3, 3);
    }
    else if (mapToLoad == MIRAGEA)
    {
        mapToSet = MIRAGEA;
        allMaps[mapToLoad].StairsCount = 7;
        allMaps[mapToLoad].AllStairs = malloc(allMaps[mapToLoad].StairsCount * sizeof(Stairs));
        CreateStairs(-0.3000029, 9.553, 38.49, 43.2, -0.043, 2.505, 3, 0);
        CreateStairs(-24.41, -17.87301, 17.02, 24.2, -0.013, 2.419, 0, 1);
        CreateStairs(-24.45, -19.562, 32.28, 40.28, 2.434, 4.89, 1, 2);
        CreateStairs(-44.49, -36.315, -24.1, -16.711, -0.043, 2.426, 2, 3);
        CreateStairs(-10.127, -1.075, -32.645, -26.56, -0.042, 2.495, 3, 4);
        CreateStairs(-34.294, -32.68, 32.373, 38.91, 4.877, 5.707, 1, 5);
        CreateStairs(-34.233, -29.37, 31.502, 33.01, 4.88, 5.697, 0, 6);
    }
}

/**
 * @brief Set bomb zone
 *
 * @param mapToLoad Map index to load (see mapEnum enum in map.h)
 */
void LoadAllBombZones(int mapToLoad)
{
    if (mapToLoad == DUST2)
    {
        // Set the number of bomb zones
        TwoSites = true;
        allMaps[mapToLoad].BombsTriggersCollisionsCount = 2;
        allMaps[mapToLoad].AllBombsTriggersCollisions = malloc(allMaps[mapToLoad].BombsTriggersCollisionsCount * sizeof(Site));

        int nearWaypoints0[12] = {2, 3, 9, 10, 11, 12, 13, 14, 15, 16, 17, 57};
        SetBombZone(40.8, -20.8, 5, 5, 0, 14, 12, nearWaypoints0); // A
        int nearWaypoints1[10] = {21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
        SetBombZone(-28.03, -27.07, 4.46785, 4.578236, 1, 29, 10, nearWaypoints1); // B
    }
    else if (mapToLoad == TUTORIAL)
    {
        // Set the number of bomb zones
        TwoSites = false;
        allMaps[mapToLoad].BombsTriggersCollisionsCount = 1;
        allMaps[mapToLoad].AllBombsTriggersCollisions = malloc(allMaps[mapToLoad].BombsTriggersCollisionsCount * sizeof(Site));

        int nearWaypoints0[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
        SetBombZone(-8.21, -1.59, 4, 4, 0, 0, 14, nearWaypoints0); // A
    }
    else if (mapToLoad == DUST2_2x2)
    {
        // Set the number of bomb zones
        TwoSites = false;
        allMaps[mapToLoad].BombsTriggersCollisionsCount = 1;
        allMaps[mapToLoad].AllBombsTriggersCollisions = malloc(allMaps[mapToLoad].BombsTriggersCollisionsCount * sizeof(Site));

        int nearWaypoints0[14] = {0,1,2,3,4,5,6,7,8,9,10,11,14,30};
        SetBombZone(40.8, -20.8, 5, 5, 0, 8, 14, nearWaypoints0); // A
    }
    else if (mapToLoad == AIM_MAP)
    {
        // Set the number of bomb zones
        TwoSites = false;
        allMaps[mapToLoad].BombsTriggersCollisionsCount = 1;
        allMaps[mapToLoad].AllBombsTriggersCollisions = malloc(allMaps[mapToLoad].BombsTriggersCollisionsCount * sizeof(Site));

        int nearWaypoints0[39] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38};
        SetBombZone(22.22, 0.12, 4, 4, 0, -1, 39, nearWaypoints0); // A
    }
    else if (mapToLoad == B2000)
    {
        // Set the number of bomb zones
        TwoSites = false;
        allMaps[mapToLoad].BombsTriggersCollisionsCount = 1;
        allMaps[mapToLoad].AllBombsTriggersCollisions = malloc(allMaps[mapToLoad].BombsTriggersCollisionsCount * sizeof(Site));

        int nearWaypoints0[26] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
        SetBombZone(-1.76, -28.5, 4, 4, 0, 24, 26, nearWaypoints0); // A
    }
    else if (mapToLoad == MIRAGEA)
    {
        // Set the number of bomb zones
        TwoSites = false;
        allMaps[mapToLoad].BombsTriggersCollisionsCount = 1;
        allMaps[mapToLoad].AllBombsTriggersCollisions = malloc(allMaps[mapToLoad].BombsTriggersCollisionsCount * sizeof(Site));

        int nearWaypoints0[13] = {0,1,2,3,4,5,13,14,15,16,17,18,31};
        SetBombZone(-36.61, -0.39, 5, 6.6, 0, 4, 13, nearWaypoints0); // A
    }
}
/**
 * @brief Create a stairs
 *
 * @param xSideA
 * @param xSideB
 * @param zSideA
 * @param zSideB
 * @param startY Start y position of the stairs
 * @param endY End y position of the stairs
 * @param direction Stairs direction
 * @param index Stairs index
 */
void CreateStairs(float xSideA, float xSideB, float zSideA, float zSideB, float startY, float endY, int direction, int index)
{
    Stairs *newStairs = &allMaps[mapToSet].AllStairs[index];
    newStairs->xSideA = xSideA;
    newStairs->xSideB = xSideB;
    newStairs->zSideA = zSideA;
    newStairs->zSideB = zSideB;
    newStairs->startY = startY;
    newStairs->endY = endY;
    newStairs->direction = direction;
}

/**
 * @brief Create a Wall
 *
 * @param xPos x position (float version)
 * @param yPos y position (float version)
 * @param zPos z position (float version)
 * @param xSize x size value (float version)
 * @param ySize y size value (float version)
 * @param zSize z size value (float version)
 * @param Zone Wall zone
 * @param index Wall index
 */
void CreateWall(float xPos, float yPos, float zPos, float xSize, float ySize, float zSize, int Zone, int index)
{
    Wall *newWall = &allMaps[mapToSet].AllWallsCollisions[index];
    // Create wall pointers
    newWall->WallModel = NE_ModelCreate(NE_Static);
    newWall->WallPhysics = NE_PhysicsCreate(NE_BoundingBox);
    newWall->WallPhysics->physicsgroupCount = 2;
    newWall->WallPhysics->physicsgroup[1] = 1;
    NE_ModelSetCoord(newWall->WallModel, xPos, yPos, zPos);               // Set wall model position
    NE_PhysicsSetModel(newWall->WallPhysics, (void *)newWall->WallModel); // Physics object and Model assigned to it
    NE_PhysicsSetSize(newWall->WallPhysics, xSize, ySize, zSize);         // Set physics size
    // Set wall position
    newWall->position.x = xPos * 8192.0;
    newWall->position.y = yPos * 8192.0;
    newWall->position.z = zPos * 8192.0;
    newWall->ZoneCollision = Zone;
    // Calculate a box from wall size values for raycasting
    newWall->WallCollisionBox.BoxXRangeA = newWall->position.x + newWall->WallPhysics->xsize;
    newWall->WallCollisionBox.BoxXRangeB = newWall->position.x - newWall->WallPhysics->xsize;
    newWall->WallCollisionBox.BoxYRangeA = newWall->position.y + newWall->WallPhysics->ysize;
    newWall->WallCollisionBox.BoxYRangeB = newWall->position.y - newWall->WallPhysics->ysize;
    newWall->WallCollisionBox.BoxZRangeA = newWall->position.z + newWall->WallPhysics->zsize;
    newWall->WallCollisionBox.BoxZRangeB = newWall->position.z - newWall->WallPhysics->zsize;

    // Disable physics
    NE_PhysicsEnable(newWall->WallPhysics, false);
}

/**
 * @brief Calculate a box from physics values for raycasting
 *
 * @param playerIndex Player index
 */
void CalculatePlayerColBox(int playerIndex)
{
    CalculatePlayerPosition(playerIndex);
    Player *player = &AllPlayers[playerIndex];

    float xSize = player->xSize;
    float ySize = player->ySize;
    float zSize = player->zSize;
    // Get min/max of the box
    player->PlayerCollisionBox.BoxXRangeA = (player->position.x + xSize) * 4096.0;
    player->PlayerCollisionBox.BoxXRangeB = (player->position.x - xSize) * 4096.0;
    player->PlayerCollisionBox.BoxYRangeA = (player->position.y + ySize) * 4096.0;
    player->PlayerCollisionBox.BoxYRangeB = (player->position.y - ySize) * 4096.0;
    player->PlayerCollisionBox.BoxZRangeA = (player->position.z + zSize) * 4096.0;
    player->PlayerCollisionBox.BoxZRangeB = (player->position.z - zSize) * 4096.0;
}

/**
 * @brief Calculate a box from physics values for raycasting
 *
 * @param position Position of the box
 * @param size Size of the box
 */
CollisionBox CalculateColBox(Vector3 position, Vector3 size)
{
    CollisionBox box;
    // Get min/max of the box
    box.BoxXRangeA = (position.x + size.x);
    box.BoxXRangeB = (position.x - size.x);
    box.BoxYRangeA = (position.y + size.y);
    box.BoxYRangeB = (position.y - size.y);
    box.BoxZRangeA = (position.z + size.z);
    box.BoxZRangeB = (position.z - size.z);
    return box;
}

/**
 * @brief Calculate a box for trigger for occlusion culling
 *
 * @param xPos x position (float version)
 * @param zPos z position (float version)
 * @param xSize x size value (float version)
 * @param zSize z size value (float version)
 * @param triggerId Trigger Id
 */
void CalculateTriggerColBox(float xPos, float zPos, float xSize, float zSize, int triggerId)
{
    CollisionBox2D *trigger = &allMaps[mapToSet].AllZones[triggerId].collisionBox;
    // Get min/max of the box
    trigger->BoxXRangeA = xPos + xSize / 2.0;
    trigger->BoxXRangeB = xPos - xSize / 2.0;
    trigger->BoxZRangeA = zPos + zSize / 2.0;
    trigger->BoxZRangeB = zPos - zSize / 2.0;
}

/**
 * @brief Calculate a box for shadows
 *
 * @param xPos x position (float version)
 * @param yPos y position (float version)
 * @param zPos z position (float version)
 * @param xSize x size value (float version)
 * @param ySize y size value (float version)
 * @param zSize z size value (float version)
 * @param triggerId Trigger Id
 */
void CalculateShadowColBox(float xPos, float yPos, float zPos, float xSize, float ySize, float zSize, int collisionBoxIndex)
{
    CollisionBox *collisionBox = &allMaps[mapToSet].AllShadowCollisionBox[collisionBoxIndex];
    // Get min/max of the box
    collisionBox->BoxXRangeA = xPos + xSize / 2.0;
    collisionBox->BoxXRangeB = xPos - xSize / 2.0;
    collisionBox->BoxYRangeA = yPos + ySize / 2.0;
    collisionBox->BoxYRangeB = yPos - ySize / 2.0;
    collisionBox->BoxZRangeA = zPos + zSize / 2.0;
    collisionBox->BoxZRangeB = zPos - zSize / 2.0;
}

/**
 * @brief Set bomb zone of the map
 *
 * @param xPos x position (float version)
 * @param zPos z position (float version)
 * @param xSize x size value (float version)
 * @param zSize z size value (float version)
 * @param triggerId Trigger Id
 * @param waypoint
 */
void SetBombZone(float xPos, float zPos, float xSize, float zSize, int triggerId, int waypoint, int nearWaypointCount, int nearWaypoint[nearWaypointCount])
{
    Site *site = &allMaps[mapToSet].AllBombsTriggersCollisions[triggerId];
    site->collisionBox.BoxXRangeA = xPos + xSize / 2.0;
    site->collisionBox.BoxXRangeB = xPos - xSize / 2.0;
    site->collisionBox.BoxZRangeA = zPos + zSize / 2.0;
    site->collisionBox.BoxZRangeB = zPos - zSize / 2.0;
    site->waypoint = waypoint;

    allMaps[mapToSet].AllBombsTriggersCollisions[triggerId].nearWaypoints = (int *)malloc(nearWaypointCount * sizeof(int));
    allMaps[mapToSet].AllBombsTriggersCollisions[triggerId].nearWaypointCount = nearWaypointCount;

    for (int i = 0; i < nearWaypointCount; i++)
    {
        allMaps[mapToSet].AllBombsTriggersCollisions[triggerId].nearWaypoints[i] = nearWaypoint[i];
    }
}

/**
 * @brief Set bomb defuse zone with bomb position
 *
 * @param xPos x position (float version)
 * @param zPos z position (float version)
 * @param defuseZoneCollisions CollisionBox2D
 */
void SetBombDefuseZone(float xPos, float zPos, CollisionBox2D *defuseZoneCollisions)
{
    defuseZoneCollisions->BoxXRangeA = xPos + 1;
    defuseZoneCollisions->BoxXRangeB = xPos - 1;
    defuseZoneCollisions->BoxZRangeA = zPos + 1;
    defuseZoneCollisions->BoxZRangeB = zPos - 1;
}

/**
 * @brief Set bomb defuse zone with bomb position
 *
 * @param xPos x position (float version)
 * @param zPos z position (float version)
 * @param defuseZoneCollisions CollisionBox2D
 */
void SetCollisionBox(float xPos, float yPos, float zPos, float xSize, float ySize, float zSize, CollisionBoxF *collisionBox)
{
    collisionBox->BoxXRangeA = xPos + xSize;
    collisionBox->BoxXRangeB = xPos - xSize;
    collisionBox->BoxYRangeA = yPos + ySize;
    collisionBox->BoxYRangeB = yPos - ySize;
    collisionBox->BoxZRangeA = zPos + zSize;
    collisionBox->BoxZRangeB = zPos - zSize;
}

/**
 * @brief Set bomb taking zone with bomb position when the bomb is dropped
 *
 * @param xPos x position (float version)
 * @param zPos z position (float version)
 * @param defuseZoneCollisions CollisionBox2D
 */
void SetBombTakingZone(float xPos, float zPos, CollisionBox2D *defuseZoneCollisions)
{
    defuseZoneCollisions->BoxXRangeA = xPos + 0.8;
    defuseZoneCollisions->BoxXRangeB = xPos - 0.8;
    defuseZoneCollisions->BoxZRangeA = zPos + 0.8;
    defuseZoneCollisions->BoxZRangeB = zPos - 0.8;
}

/**
 * @brief Check in which zone the player is
 *
 * @param playerIndex player to check index
 */
void checkPlayerOcclusionZone(int playerIndex)
{
    // If the player is the same zone, do nothing
    if (AllPlayers[playerIndex].CurrentOcclusionZone < allMaps[currentMap].zonesCount && AllPlayers[playerIndex].position.x <= allMaps[currentMap].AllZones[AllPlayers[playerIndex].CurrentOcclusionZone].collisionBox.BoxXRangeA && AllPlayers[playerIndex].position.x >= allMaps[currentMap].AllZones[AllPlayers[playerIndex].CurrentOcclusionZone].collisionBox.BoxXRangeB && AllPlayers[playerIndex].position.z <= allMaps[currentMap].AllZones[AllPlayers[playerIndex].CurrentOcclusionZone].collisionBox.BoxZRangeA && AllPlayers[playerIndex].position.z >= allMaps[currentMap].AllZones[AllPlayers[playerIndex].CurrentOcclusionZone].collisionBox.BoxZRangeB)
    {
    }
    else
    {
        // Or find the new zone

        // Check occlusion zones
        for (int TriggersIndex = 0; TriggersIndex < allMaps[currentMap].zonesCount; TriggersIndex++)
        {
            if (AllPlayers[playerIndex].position.x <= allMaps[currentMap].AllZones[TriggersIndex].collisionBox.BoxXRangeA && AllPlayers[playerIndex].position.x >= allMaps[currentMap].AllZones[TriggersIndex].collisionBox.BoxXRangeB && AllPlayers[playerIndex].position.z <= allMaps[currentMap].AllZones[TriggersIndex].collisionBox.BoxZRangeA && AllPlayers[playerIndex].position.z >= allMaps[currentMap].AllZones[TriggersIndex].collisionBox.BoxZRangeB)
            {
                AllPlayers[playerIndex].CurrentOcclusionZone = TriggersIndex;
                break;
            }
        }
    }
}

/**
 * @brief Check if player is in a shadow zone
 *
 * @param playerIndex player to check index
 */
void checkPlayerShadow(int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    float correctedPlayerYValue = player->position.y + player->ySize / 2.0;
    // If the player is the same zone, do nothing
    if (player->currentShadowCollBox < allMaps[currentMap].shadowCollisionBoxCount && player->position.x <= allMaps[currentMap].AllShadowCollisionBox[player->currentShadowCollBox].BoxXRangeA && player->position.x >= allMaps[currentMap].AllShadowCollisionBox[player->currentShadowCollBox].BoxXRangeB && correctedPlayerYValue <= allMaps[currentMap].AllShadowCollisionBox[player->currentShadowCollBox].BoxYRangeA && correctedPlayerYValue >= allMaps[currentMap].AllShadowCollisionBox[player->currentShadowCollBox].BoxYRangeB && player->position.z <= allMaps[currentMap].AllShadowCollisionBox[player->currentShadowCollBox].BoxZRangeA && player->position.z >= allMaps[currentMap].AllShadowCollisionBox[player->currentShadowCollBox].BoxZRangeB)
    {
        player->inShadow = true;
    }
    else
    {
        // Or find the new zone
        player->inShadow = false;
        //  Check occlusion zones
        for (int shadowIndex = 0; shadowIndex < allMaps[currentMap].shadowCollisionBoxCount; shadowIndex++)
        {
            if (player->position.x <= allMaps[currentMap].AllShadowCollisionBox[shadowIndex].BoxXRangeA && player->position.x >= allMaps[currentMap].AllShadowCollisionBox[shadowIndex].BoxXRangeB && correctedPlayerYValue <= allMaps[currentMap].AllShadowCollisionBox[shadowIndex].BoxYRangeA && correctedPlayerYValue >= allMaps[currentMap].AllShadowCollisionBox[shadowIndex].BoxYRangeB && player->position.z <= allMaps[currentMap].AllShadowCollisionBox[shadowIndex].BoxZRangeA && player->position.z >= allMaps[currentMap].AllShadowCollisionBox[shadowIndex].BoxZRangeB)
            {
                player->inShadow = true;
                player->currentShadowCollBox = shadowIndex;
                return;
            }
        }
    }
}

/**
 * @brief Check where the bomb is
 *
 * @return int Waypoint index where the bomb is
 */
int checkBombZoneWaypoint()
{
    for (int TriggersIndex = 0; TriggersIndex < allMaps[currentMap].BombsTriggersCollisionsCount; TriggersIndex++)
    {
        Site *site = &allMaps[currentMap].AllBombsTriggersCollisions[TriggersIndex];
        if (localPlayer->position.x <= site->collisionBox.BoxXRangeA && localPlayer->position.x >= site->collisionBox.BoxXRangeB && localPlayer->position.z <= site->collisionBox.BoxZRangeA && localPlayer->position.z >= site->collisionBox.BoxZRangeB)
        {
            return site->waypoint;
        }
    }
    return -1;
}

/**
 * @brief Check all zones (triggers for occlusion culling, bomb pland and defuse trigger)
 *
 * @param bombDefuseZone
 * @param CanPutBomb
 * @param canDefuseBomb
 */
void CheckZones(CollisionBox2D bombDefuseZone, bool *CanPutBomb, bool *canDefuseBomb)
{
    // Check players occlusion zones and shadows zones
    for (int i = 0; i < MaxPlayer; i++)
    {
        if (AllPlayers[i].Id != UNUSED)
        {
            checkPlayerOcclusionZone(i);
            checkPlayerShadow(i);
        }
    }

    if (!bombDropped)
    {
        // Check bomb zones
        for (int TriggersIndex = 0; TriggersIndex < allMaps[currentMap].BombsTriggersCollisionsCount; TriggersIndex++)
        {
            Site *site = &allMaps[currentMap].AllBombsTriggersCollisions[TriggersIndex];
            if (localPlayer->position.x <= site->collisionBox.BoxXRangeA && localPlayer->position.x >= site->collisionBox.BoxXRangeB && localPlayer->position.z <= site->collisionBox.BoxZRangeA && localPlayer->position.z >= site->collisionBox.BoxZRangeB)
            {
                *CanPutBomb = true;
                break;
            }
        }

        if (localPlayer->position.x <= bombDefuseZone.BoxXRangeA && localPlayer->position.x >= bombDefuseZone.BoxXRangeB && localPlayer->position.z <= bombDefuseZone.BoxZRangeA && localPlayer->position.z >= bombDefuseZone.BoxZRangeB)
            *canDefuseBomb = true;
    }
    else
    {
        *canDefuseBomb = false;
        *CanPutBomb = false;
    }
}

/**
 * @brief Check if players is on the dropped bomb to take it
 *
 * @param bombDefuseZone
 */
void checkTakingBombZone(CollisionBox2D bombDefuseZone)
{
    if (!bombDropped)
        return;

    for (int i = 0; i < MaxPlayer; i++)
    {
        // Check only for the local player in online mode
        if (i != 0 && Connection != OFFLINE)
            break;

        Player *player = &AllPlayers[i];
        if (player->IsDead || player->Id == UNUSED || player->Team == COUNTERTERRORISTS)
            continue;

        // If the player is on the bomb
        if (player->position.x <= bombDefuseZone.BoxXRangeA && player->position.x >= bombDefuseZone.BoxXRangeB && player->position.z <= bombDefuseZone.BoxZRangeA && player->position.z >= bombDefuseZone.BoxZRangeB)
        {
            // Add the bomb in the player's inventory
            player->haveBomb = true;
            SetGunInInventoryForNonLocalPlayer(i, 28, 8);
            bombDropped = false;

            // Send the message to the server
            SendGetDroppedBomb = true;
            // Force player to send position
            SendPosition = true;
            SendPositionData = 0;

            // Disable bomb finding for all other player
            for (int playerIndex2 = 0; playerIndex2 < MaxPlayer; playerIndex2++)
            {
                Player *player2 = &AllPlayers[playerIndex2];
                player2->searchForDroppedBomb = false;
            }
            break;
        }
    }
}

/**
 * @brief Check if local player is in the shop zone
 *
 */
void checkShopZone()
{
    if (localPlayer->position.x <= shopZone.BoxXRangeA && localPlayer->position.x >= shopZone.BoxXRangeB && localPlayer->position.y <= shopZone.BoxYRangeA && localPlayer->position.y >= shopZone.BoxYRangeB && localPlayer->position.z <= shopZone.BoxZRangeA && localPlayer->position.z >= shopZone.BoxZRangeB)
    {
        isInShopZone = true;
    }
    else if (isInShopZone)
    {
        isInShopZone = false;
        if (currentMenu == SHOP || currentMenu == SHOPCATEGORIES)
        {
            initGameMenu();
        }
    }
}

// Last stairs where the player was
int LastStairs = 0;

/**
 * @brief check if local player is on a ramp/stairs
 *
 * @param canJump
 * @param isInDownStairs
 */
void CheckStairs(int *canJump, bool *isInDownStairs)
{
    bool firstScan = true;
    // To optimize, start scan on old stairs to avoid checking all other stairs
    for (int i = LastStairs; i < allMaps[currentMap].StairsCount; i++)
    {
        Stairs *stairs = &allMaps[currentMap].AllStairs[i];
        // If the localplayer is on a stairs
        if (localPlayer->position.z >= stairs->zSideA && localPlayer->position.z <= stairs->zSideB && localPlayer->position.x >= stairs->xSideA && localPlayer->position.x <= stairs->xSideB)
        {
            // Get the player y position
            float yVal = 0;
            if (stairs->direction == 0)
                yVal = map(localPlayer->position.z, stairs->zSideA, stairs->zSideB, stairs->endY, stairs->startY);
            else if (stairs->direction == 1)
                yVal = map(localPlayer->position.x, stairs->xSideA, stairs->xSideB, stairs->endY, stairs->startY);
            else if (stairs->direction == 2)
                yVal = map(localPlayer->position.z, stairs->zSideA, stairs->zSideB, stairs->startY, stairs->endY);
            else
                yVal = map(localPlayer->position.x, stairs->xSideA, stairs->xSideB, stairs->startY, stairs->endY);

            if (localPlayer->position.y < yVal && yVal - localPlayer->position.y < 3)
            {
                // Stop player Y movements
                if (localPlayer->PlayerPhysic->yspeed < 100)
                    localPlayer->PlayerPhysic->yspeed = 0;

                // Change the player position
                localPlayer->position.y = yVal;
                NE_ModelSetCoord(localPlayer->PlayerModel, localPlayer->position.x, localPlayer->position.y, localPlayer->position.z);
                // Player will be able to jump because he is touching the ground
                *canJump = 10;
            }
            if (localPlayer->position.y == yVal)
                *isInDownStairs = true;

            LastStairs = i;
            break;
        }
        // If there is no stairs found on the first scan, restard a scan and check all stairs
        if (firstScan)
        {
            firstScan = false;
            LastStairs = 0;
            i = 0;
        }
    }
}

/**
 * @brief Check if a player is in a zone for occlusion culling
 *
 * @param zone Zone data
 * @param posx x player position (Int version)
 * @param posz x player position (Int version)
 * @return true
 * @return false
 */
bool checkZoneForOcclusion(OcclusionZone *zone, int posx, int posz)
{
    if (zone->anglesInt[0].x >= posx && zone->anglesInt[2].x <= posx)
    {
        if (zone->anglesInt[0].y >= posz && zone->anglesInt[2].y <= posz)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief Check if grenade is on a ramp
 *
 * @param grenade Grenade data
 * @return true
 * @return false
 */
bool CheckStairsForGrenades(PhysicalGrenade *grenade)
{
    bool firstScan = true;
    for (int i = grenade->LastStairs; i < allMaps[currentMap].StairsCount; i++)
    {
        float xpos = grenade->Model->x / 4096.0;
        float ypos = grenade->Model->y / 4096.0;
        float zpos = grenade->Model->z / 4096.0;
        Stairs *stairs = &allMaps[currentMap].AllStairs[i];
        // If the grenade is on a stairs
        if (zpos >= stairs->zSideA && zpos <= stairs->zSideB && xpos >= stairs->xSideA && xpos <= stairs->xSideB)
        {
            // Get the grenade y position
            float yVal = 0;
            if (stairs->direction == 0)
            {
                yVal = map(zpos, stairs->zSideA, stairs->zSideB, stairs->endY, stairs->startY);
            }
            else if (stairs->direction == 1)
            {
                yVal = map(xpos, stairs->xSideA, stairs->xSideB, stairs->endY, stairs->startY);
            }
            else if (stairs->direction == 2)
            {
                yVal = map(zpos, stairs->zSideA, stairs->zSideB, stairs->startY, stairs->endY);
            }
            else
            {
                yVal = map(xpos, stairs->xSideA, stairs->xSideB, stairs->startY, stairs->endY);
            }

            if (ypos < yVal && yVal - ypos < 3)
            {
                // Stop player Y movements
                if (grenade->Physic->yspeed < 100)
                    grenade->Physic->yspeed = 0;

                // Change the grenade position
                NE_ModelSetCoord(grenade->Model, xpos, yVal - 0.6, zpos);

                if (stairs->startY != stairs->endY)
                {
                    // Make the grenade roll to the bottom of the ramp
                    if (stairs->direction == 0)
                    {
                        grenade->Physic->zspeed += 15;
                    }
                    else if (stairs->direction == 1)
                    {
                        grenade->Physic->xspeed += 15;
                    }
                    else if (stairs->direction == 2)
                    {
                        grenade->Physic->zspeed -= 15;
                    }
                    else
                    {
                        grenade->Physic->xspeed -= 15;
                    }
                }
            }
            grenade->LastStairs = i;
            return true;
        }
        // If there is no stairs found on the first scan, restard a scan and check all stairs
        if (firstScan)
        {
            firstScan = false;
            grenade->LastStairs = 0;
            i = 0;
        }
    }
    return false;
}

// Optimized Min function
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
// Optimized Max function
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Get the distance bewteen the start position and the hitted object
 *
 * @param corner1 Positon of the bottom left corner of the object to hit
 * @param corner2 Positon of the top right corner of the object to hit
 * @param dirfrac Direction
 * @param startPosition Start position of the raycast
 * @param t Distance found
 * @return Object hitted? True : Yes, False : no
 */
bool getHitDistance(Vector3Int corner1, Vector3Int corner2, Vector3 dirfrac, Vector3Int startPosition, float *t)
{
    int t1 = (corner1.x - startPosition.x) * dirfrac.x;
    int t2 = (corner2.x - startPosition.x) * dirfrac.x;
    int t3 = (corner1.y - startPosition.y) * dirfrac.y;
    int t4 = (corner2.y - startPosition.y) * dirfrac.y;
    int t5 = (corner1.z - startPosition.z) * dirfrac.z;
    int t6 = (corner2.z - startPosition.z) * dirfrac.z;

    int tmin = MAX(MAX(MIN(t1, t2), MIN(t3, t4)), MIN(t5, t6));
    int tmax = MIN(MIN(MAX(t1, t2), MAX(t3, t4)), MAX(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us, if tmin > tmax, ray doesn't intersect AABB
    if (tmax < 0 || tmin > tmax)
    {
    }
    else
    {
        if (*t > tmin)
        {
            *t = tmin;
            return true;
        }
    }
    return false;
}

/**
 * @brief Raycast function
 *
 * @param p1x
 * @param p1y
 * @param p2x
 * @param p2y
 * @param p3x
 * @param p3y
 * @return float
 */
float sign(float p1x, float p1y, float p2x, float p2y, float p3x, float p3y)
{
    return (p1x - p3x) * (p2y - p3y) - (p2x - p3x) * (p1y - p3y);
}

/**
 * @brief Check if a point is in a 2D triangle
 *
 * @param px x position (float position) of the point to check
 * @param py y position (float position) of the point to check
 * @param p1x x position (float version) of the corner 1 of the triangle
 * @param p1y y position (float version) of the corner 1 of the triangle
 * @param p2x x position (float version) of the corner 2 of the triangle
 * @param p2y y position (float version) of the corner 2 of the triangle
 * @param p3x x position (float version) of the corner 3 of the triangle
 * @param p3y y position (float version) of the corner 3 of the triangle
 * @return is the point in the triangle?
 */
bool PointInTriangle(float px, float py, float p1x, float p1y, float p2x, float p2y, float p3x, float p3y)
{
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = sign(px, py, p1x, p1y, p2x, p2y);
    d2 = sign(px, py, p2x, p2y, p3x, p3y);
    d3 = sign(px, py, p3x, p3y, p1x, p1y);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

/**
 * @brief  Raycast function int version
 *
 * @param p1x
 * @param p1y
 * @param p2x
 * @param p2y
 * @param p3x
 * @param p3y
 * @return int
 */
int signInt(int p1x, int p1y, int p2x, int p2y, int p3x, int p3y)
{
    return (p1x - p3x) * (p2y - p3y) - (p2x - p3x) * (p1y - p3y);
}

/**
 * @brief Check if a point is in a 2D triangle (Int version, better performance)
 *
 * @param px x position (int position) of the point to check
 * @param py y position (int position) of the point to check
 * @param p1x x position (int version) of the corner 1 of the triangle
 * @param p1y y position (int version) of the corner 1 of the triangle
 * @param p2x x position (int version) of the corner 2 of the triangle
 * @param p2y y position (int version) of the corner 2 of the triangle
 * @param p3x x position (int version) of the corner 3 of the triangle
 * @param p3y y position (int version) of the corner 3 of the triangle
 * @return is the point in the triangle?
 */
bool PointInTriangleInt(int px, int py, int p1x, int p1y, int p2x, int p2y, int p3x, int p3y)
{
    int d1, d2, d3;
    bool has_neg, has_pos;

    d1 = sign(px, py, p1x, p1y, p2x, p2y);
    d2 = sign(px, py, p2x, p2y, p3x, p3y);
    d3 = sign(px, py, p3x, p3y, p1x, p1y);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}