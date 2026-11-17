// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021-2022, Fewnity - Grégory Machefer
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

#ifndef AI_H_ /* Include guard */
#define AI_H_

#include "../main.h"

typedef struct
{
    float x;
    float y;
    float z;
    int edgeCount;
    int *edge;
} Waypoint;

typedef struct
{
    int **matrixOneLength;
} PathLength;

#define BOTS_NAMES_COUNT 46
extern const char *botsNames[BOTS_NAMES_COUNT];
extern bool botsNamesTaken[BOTS_NAMES_COUNT];
extern PathLength *AllMatricesLength;
extern int MatriceCount;
extern int waypointsSize;
extern int MatricesSize;

extern bool equalTeam;

extern int SiteAPoint;
extern int SiteBPoint;
extern int TwoSites;

// Plan of the terrorist bots for the round, on defuse maps with two bomb sites
enum TerroristsStrategy
{
    T_STRATEGY_NONE = 0,  // One site or not a defuse map
    T_STRATEGY_RUSH_A,    // Everyone goes to site A
    T_STRATEGY_RUSH_B,    // Everyone goes to site B
    T_STRATEGY_SPLIT,     // Bots spread on both sites, the bomber goes to a random site
};
extern enum TerroristsStrategy terroristsStrategy;
void ChooseTerroristsStrategy();
extern int amountOfBots;

void StartChecking(int playerIndex, int FinalWaypoint);
void GetRandomPoint(int currentAiToCheck);
void CreateWaypoint(int id, float x, float y, float z, int edgeCount, int *edge);
void CheckPathWaypoint(int playerIndex, int StartWaypoint, int FinalWaypoint);
int GetDistanceBewteenTwoPlayers(int player1Index, int player2Index);
int GetDistanceBewteenPlayerAndWaypoint(int playerIndex, int waypointIndex);
void copyArrayToAllMatricesLength(int size, const int matrix[size][size], int index);
int getNearestWaypoint(float x, float y, float z);
void ResetTakenBotsNames();
void CheckShopForBot(int playerId);
void checkShopForBots();
void AiCheckForAction();
void checkAiShoot();
void SetDefuser(int defuserIndex);
void SetRandomDefuser();
void freeAllMatricesLength(int size);
void freeWaypoint();

void setBotAmount4();
void setBotAmount6();
void setBotAmount10();

Waypoint *GetWaypoints();

#endif // AI_H_