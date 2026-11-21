// SPDX-License-Identifier: MIT
//
// Copyright (c) 2026, Counter-Strike DS contributors
//
// Local multiplayer (NiFi), host side.
//
// The host plays a normal single player game without bots: the clients are
// "network players" (not AI, moved by their positions like online). Every
// frame, the host compares the state of the game with what it already sent
// and sends the changes to the clients with the messages of the online server
// (POS, SETHEALTH, TEAM, INVTORY, PartyRound, TimerA...).
//
// The clients send the messages of the online client (numbers, see
// RequestType in network.h): POS, SHOOT, HIT, CURGUN, BUY, RELOADED...
//
// Bomb, grenades and dropped guns are synchronized too. No votes: the host
// chooses the map and the mode.

#include "main.h"
#include "localhost.h"
#include "localplay.h"
#include "network.h"
#include "player.h"
#include "party.h"
#include "gun.h"
#include "grenade.h"
#include "equipment.h"
#include "ui.h"
#include "ai.h"
#include "droppedGuns.h"
#include "collisions.h"

// Player id of each client (peer index + 1, the host is 0)
#define PEER_PLAYER_ID(peerIndex) ((peerIndex) + 1)

// Positions of the players sent every 3 frames (20 times per second), and
// at least once per second when the player doesn't move
#define POSITION_SEND_DELAY 3
#define POSITION_RESEND_FRAMES 60

// Frames during which the positions sent by a client are ignored after a
// teleportation (spawn), the client still sends its old position
#define TELEPORT_IGNORE_FRAMES 20

typedef struct
{
    bool known;
    int health;
    int armor;
    bool haveHeadset;
    int team;
    int killCount;
    int deathCount;
    int currentGun;
    int inventory[inventoryCapacity];
    int money;
    bool isDead;
} SentPlayerState;

static bool hostGameRunning = false;
static int savedAmountOfBots = 0;
static SentPlayerState sentPlayers[MaxPlayer];
static int sentRoundState = -1;
static int sentCounterScore = -1;
static int sentTerroristsScore = -1;
static int sentMinutes = -1;
static int sentSeconds = -1;
static int positionTimer = 0;
static int ignorePositionFrames[MaxPlayer];
static char sentPositions[MaxPlayer][96]; // Last position sent of each player
static int sentPositionAge[MaxPlayer];

// Bomb
static int sentBombCarrier = -2;    // Player index, -1 nobody, -2 not sent yet
static bool sentBombDropped = false;
static float sentDroppedBombX = 0, sentDroppedBombZ = 0;
static bool sentBombPlanted = false;
static bool sentBombDefused = false;
static int bombPlanterIndex = NO_PLAYER; // Client who planted (already knows it)
static int bombDefuserIndex = NO_PLAYER; // Client who defused (already knows it)

// Dropped guns: age of the gun sent in each slot (-1: none)
static int sentDroppedGunAge[MAX_DROPPED_GUNS];

static bool sentPartyFinished = false;

bool isLocalHostGame(void)
{
    return hostGameRunning;
}

static int getPlayerIndexById(int id)
{
    for (int i = 0; i < MaxPlayer; i++)
    {
        if (AllPlayers[i].Id == id)
            return i;
    }
    return NO_PLAYER;
}

// Peer index of a player, -1 for the host
static int getPeerOfPlayer(int playerIndex)
{
    int id = AllPlayers[playerIndex].Id;
    if (id <= 0)
        return -1;
    return id - 1;
}

/**
 * @brief Send a message to all the clients except the client of a player
 *
 */
static void sendToAllExcept(int playerIndex, const char *text)
{
    int exceptPeer = playerIndex == NO_PLAYER ? -1 : getPeerOfPlayer(playerIndex);
    for (int peer = 0; peer < LOCAL_MAX_CLIENTS; peer++)
    {
        if (peer != exceptPeer && localPeerIsInGame(peer))
            localSendToPeer(peer, text);
    }
}

static void sendToPlayer(int playerIndex, const char *text)
{
    int peer = getPeerOfPlayer(playerIndex);
    if (peer >= 0 && localPeerIsInGame(peer))
        localSendToPeer(peer, text);
}

static void formatPosition(char *text, int size, int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    snprintf(text, size, "{POS;%d;%d;%d;%d;%d;%d}", player->Id,
             (int)(player->position.x * 4096), (int)(player->position.y * 4096), (int)(player->position.z * 4096),
             (int)player->Angle, (int)player->cameraAngle);
}

//////////////////////////////////////////////////////////////////////////////
// Start and end

/**
 * @brief Add the players of the room in the game (instead of the bots)
 *
 */
void localHostAddPlayers(void)
{
    // No bots: the AI code uses the indexes 1 to amountOfBots - 1
    savedAmountOfBots = amountOfBots;
    amountOfBots = 1;

    for (int peer = 0; peer < LOCAL_MAX_CLIENTS; peer++)
    {
        if (!localPeerIsInGame(peer))
            continue;

        int index = AddNewPlayer(PEER_PLAYER_ID(peer), false, false);
        if (index >= 0 && index < MaxPlayer)
        {
            AllPlayers[index].isAi = false;
            strncpy(AllPlayers[index].name, localPeerName(peer), PLAYER_MAX_LENGTH - 1);
            AllPlayers[index].name[PLAYER_MAX_LENGTH - 1] = '\0';
        }
    }
}

/**
 * @brief The game starts: send the map, the mode and the players to the
 * clients
 *
 */
void localHostStartGame(void)
{
    hostGameRunning = true;
    localState = LOCAL_PLAYING;
    // The clients load the map now
    localGiveLoadingTime();

    for (int i = 0; i < MaxPlayer; i++)
    {
        sentPlayers[i].known = false;
        ignorePositionFrames[i] = 0;
        sentPositions[i][0] = '\0';
        sentPositionAge[i] = 0;
    }
    sentRoundState = -1;
    sentCounterScore = -1;
    sentTerroristsScore = -1;
    sentMinutes = -1;
    sentSeconds = -1;
    positionTimer = 0;
    sentBombCarrier = -2;
    sentBombDropped = false;
    sentBombPlanted = false;
    sentBombDefused = false;
    bombPlanterIndex = NO_PLAYER;
    bombDefuserIndex = NO_PLAYER;
    sentPartyFinished = false;
    for (int i = 0; i < MAX_DROPPED_GUNS; i++)
        sentDroppedGunAge[i] = -1;

    for (int peer = 0; peer < LOCAL_MAX_CLIENTS; peer++)
    {
        if (!localPeerIsInGame(peer))
            continue;

        int playerId = PEER_PLAYER_ID(peer);
        char text[160];

        snprintf(text, sizeof(text), "{LSTART}{SETMODE;%d}{SETMAP;%d}{SETID;%d}", currentPartyMode, currentMap, playerId);
        localSendToPeer(peer, text);

        // The other players
        snprintf(text, sizeof(text), "{ADDRANGE");
        for (int i = 0; i < MaxPlayer; i++)
        {
            if (AllPlayers[i].Id != UNUSED && AllPlayers[i].Id != playerId)
                snprintf(text + strlen(text), sizeof(text) - strlen(text), ";%d", AllPlayers[i].Id);
        }
        snprintf(text + strlen(text), sizeof(text) - strlen(text), "}");
        localSendToPeer(peer, text);

        for (int i = 0; i < MaxPlayer; i++)
        {
            if (AllPlayers[i].Id == UNUSED)
                continue;
            snprintf(text, sizeof(text), "{SETNAME;%d;%s}", AllPlayers[i].Id, AllPlayers[i].name);
            localSendToPeer(peer, text);
        }
        // The other values (team, health, inventory, round...) are sent by
        // localHostUpdate() (first frame: nothing was sent yet)
        localSendToPeer(peer, "{ENDUPDATE}");
    }
}

/**
 * @brief End of the local game on the host
 *
 */
void localHostEndGame(void)
{
    if (!hostGameRunning)
        return;
    hostGameRunning = false;
    amountOfBots = savedAmountOfBots;

    // Everybody goes back to the room
    if (localState == LOCAL_PLAYING)
        localReturnToLobby();
}

//////////////////////////////////////////////////////////////////////////////
// Changes of the game sent to the clients

static void sendInventory(int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    char text[96];
    snprintf(text, sizeof(text), "{INVTORY;%d", player->Id);
    for (int i = 0; i < inventoryCapacity; i++)
        snprintf(text + strlen(text), sizeof(text) - strlen(text), ";%d", player->AllGunsInInventory[i]);
    snprintf(text + strlen(text), sizeof(text) - strlen(text), "}");
    sendToAllExcept(NO_PLAYER, text);

    // After the inventory: the game resets the ammo when it gets the inventory
    snprintf(text, sizeof(text), "{AMMO;%d;%d;%d;%d;%d}", player->Id,
             player->AllAmmoMagazine[0].AmmoCount, player->AllAmmoMagazine[0].TotalAmmoCount,
             player->AllAmmoMagazine[1].AmmoCount, player->AllAmmoMagazine[1].TotalAmmoCount);
    sendToAllExcept(NO_PLAYER, text);
}

static void sendPlayerChanges(int playerIndex, bool newRound)
{
    Player *player = &AllPlayers[playerIndex];
    SentPlayerState *sent = &sentPlayers[playerIndex];
    char text[96];

    if (player->Id == UNUSED)
    {
        sent->known = false;
        return;
    }

    bool first = !sent->known;
    sent->known = true;
    int sentTeamBefore = sent->team;

    if (first || sent->team != player->Team)
    {
        sent->team = player->Team;
        snprintf(text, sizeof(text), "{TEAM;%d;%d}", player->Id, player->Team);
        sendToAllExcept(NO_PLAYER, text);
    }

    if (first || sent->health != player->Health || sent->armor != player->armor || sent->haveHeadset != player->haveHeadset)
    {
        sent->health = player->Health;
        sent->armor = player->armor;
        sent->haveHeadset = player->haveHeadset;
        snprintf(text, sizeof(text), "{SETHEALTH;%d;%d;%d;%d}", player->Id, player->Health < 0 ? 0 : player->Health,
                 player->armor, player->haveHeadset ? 1 : 0);
        sendToAllExcept(NO_PLAYER, text);
    }

    // Respawn (warm up, Deathmatch) or new round: put the client at its spawn
    bool respawned = sent->isDead && !player->IsDead;
    bool teamChanged = !first && sentTeamBefore != player->Team;
    if (first || ((respawned || newRound) && !player->IsDead))
    {
        formatPosition(text, sizeof(text), playerIndex);
        sendToPlayer(playerIndex, text);
        ignorePositionFrames[playerIndex] = TELEPORT_IGNORE_FRAMES;
    }

    // Shop zone of the client (at its spawn, like the online server) and shop
    // timer: after TEAM, the client uses its team to find the zone
    if (first || teamChanged || newRound || respawned)
        sendToPlayer(playerIndex, "{SETSHOPZONE}");
    sent->isDead = player->IsDead;

    if (first || sent->killCount != player->KillCount || sent->deathCount != player->DeathCount)
    {
        sent->killCount = player->KillCount;
        sent->deathCount = player->DeathCount;
        snprintf(text, sizeof(text), "{SCRBOARD;%d;%d;%d}", player->Id, player->KillCount, player->DeathCount);
        sendToAllExcept(NO_PLAYER, text);
    }

    bool inventoryChanged = first;
    for (int i = 0; i < inventoryCapacity; i++)
    {
        if (sent->inventory[i] != player->AllGunsInInventory[i])
        {
            sent->inventory[i] = player->AllGunsInInventory[i];
            inventoryChanged = true;
        }
    }
    if (inventoryChanged)
        sendInventory(playerIndex);

    // The gun in the hands. A change asked by a client is already marked as
    // sent (see CURGUN in localHostTreatClientMessage()): a change here comes
    // from the host (buy, new round, picked up or dropped gun), the client
    // gets it too. After the inventory: the slot must not be empty.
    if (first || sent->currentGun != player->currentGunInInventory)
    {
        sent->currentGun = player->currentGunInInventory;
        snprintf(text, sizeof(text), "{CURGUN;%d;%d}", player->Id, player->currentGunInInventory);
        sendToAllExcept(NO_PLAYER, text);
    }

    // Money: only for the client
    if (first || sent->money != player->Money)
    {
        sent->money = player->Money;
        snprintf(text, sizeof(text), "{SETMONEY;%d}", player->Money);
        sendToPlayer(playerIndex, text);
    }
}

/**
 * @brief Send the changes of the game to the clients (every frame)
 *
 */
void localHostUpdate(void)
{
    if (!hostGameRunning)
        return;

    char text[96];

    // Round state, score and timer
    bool newRound = false;
    if (sentRoundState != roundState)
    {
        newRound = sentRoundState != -1 && roundState == WAIT_START;
        sentRoundState = roundState;
        snprintf(text, sizeof(text), "{PartyRound;%d}", roundState);
        sendToAllExcept(NO_PLAYER, text);
    }
    if (sentCounterScore != CounterScore || sentTerroristsScore != TerroristsScore)
    {
        sentCounterScore = CounterScore;
        sentTerroristsScore = TerroristsScore;
        snprintf(text, sizeof(text), "{SCORE;%d;%d}", CounterScore, TerroristsScore);
        sendToAllExcept(NO_PLAYER, text);
    }
    if (sentMinutes != PartyMinutes || sentSeconds != PartySeconds)
    {
        sentMinutes = PartyMinutes;
        sentSeconds = PartySeconds;
        snprintf(text, sizeof(text), "{TimerA;%d;%d}", PartyMinutes < 0 ? 0 : PartyMinutes, PartySeconds < 0 ? 0 : PartySeconds);
        sendToAllExcept(NO_PLAYER, text);
    }

    // Players
    for (int i = 0; i < MaxPlayer; i++)
    {
        sendPlayerChanges(i, newRound);
        if (ignorePositionFrames[i] > 0)
            ignorePositionFrames[i]--;
    }

    // Bomb
    {
        int carrier = -1;
        for (int i = 0; i < MaxPlayer; i++)
        {
            if (AllPlayers[i].Id != UNUSED && AllPlayers[i].haveBomb)
            {
                carrier = i;
                break;
            }
        }
        if (carrier != sentBombCarrier)
        {
            sentBombCarrier = carrier;
            if (carrier >= 0)
                snprintf(text, sizeof(text), "{SETBOMB;%d;1}", AllPlayers[carrier].Id);
            else
                snprintf(text, sizeof(text), "{SETBOMB;-1;0}");
            sendToAllExcept(NO_PLAYER, text);
        }

        if (bombDropped && (!sentBombDropped || sentDroppedBombX != droppedBombPositionAndRotation.x || sentDroppedBombZ != droppedBombPositionAndRotation.z))
        {
            sentDroppedBombX = droppedBombPositionAndRotation.x;
            sentDroppedBombZ = droppedBombPositionAndRotation.z;
            snprintf(text, sizeof(text), "{BOMBPLACE;%d;%d;%d;%d;1}", (int)(droppedBombPositionAndRotation.x * 4096),
                     (int)(droppedBombPositionAndRotation.y * 4096), (int)(droppedBombPositionAndRotation.z * 4096),
                     (int)droppedBombPositionAndRotation.r);
            sendToAllExcept(NO_PLAYER, text);
        }
        sentBombDropped = bombDropped;

        if (BombPlanted && !sentBombPlanted)
        {
            snprintf(text, sizeof(text), "{BOMBPLACE;%d;%d;%d;%d;0}", (int)(BombPosition.x * 4096), (int)(BombPosition.y * 4096),
                     (int)(BombPosition.z * 4096), (int)BombPosition.r);
            sendToAllExcept(bombPlanterIndex, text);
        }
        if (!BombPlanted)
            bombPlanterIndex = NO_PLAYER;
        sentBombPlanted = BombPlanted;

        if (BombDefused && !sentBombDefused)
            sendToAllExcept(bombDefuserIndex, "{BOMBDEFUSE}");
        if (!BombDefused)
            bombDefuserIndex = NO_PLAYER;
        sentBombDefused = BombDefused;

        // Planting or defusing sound of the host
        if (SendBombPlacing)
        {
            SendBombPlacing = false;
            snprintf(text, sizeof(text), "{BOMBPLACING;%d}", localPlayer->Id);
            sendToAllExcept(0, text);
        }
    }

    // Grenade of the host
    if (SendGrenade)
    {
        SendGrenade = false;
        snprintf(text, sizeof(text), "{GRENADE;%f;%f;%f;%d;%d;%d;%d}", x, y, z,
                 localPlayer->PlayerModel->x, localPlayer->PlayerModel->y, localPlayer->PlayerModel->z, lastThrownGrenadeType);
        sendToAllExcept(0, text);
    }

    // Guns on the floor (the age of a gun is its id for the clients)
    for (int i = 0; i < MAX_DROPPED_GUNS; i++)
    {
        DroppedGun *gun = &droppedGuns[i];
        int age = gun->used ? gun->age : -1;
        if (age == sentDroppedGunAge[i])
            continue;

        if (sentDroppedGunAge[i] >= 0)
        {
            snprintf(text, sizeof(text), "{REMOVEGUN;%d}", sentDroppedGunAge[i]);
            sendToAllExcept(NO_PLAYER, text);
        }
        if (gun->used)
        {
            snprintf(text, sizeof(text), "{DROPGUN;%d;%d;%d;%d;%d}", gun->age, gun->gunId,
                     (int)(gun->x * 4096), (int)(gun->y * 4096), (int)(gun->z * 4096));
            sendToAllExcept(NO_PLAYER, text);
        }
        sentDroppedGunAge[i] = age;
    }

    // End of the game: final scores
    if (partyFinished && !sentPartyFinished)
        sendToAllExcept(NO_PLAYER, "{ENDGAME}");
    sentPartyFinished = partyFinished;

    // Shot of the host (sound and animation for the clients)
    if (SendShoot)
    {
        SendShoot = false;
        snprintf(text, sizeof(text), "{SHOOT;%d;%d}", localPlayer->Id, getPlayerCurrentGunIndex(localPlayer));
        sendToAllExcept(0, text);
    }

    // Positions, 20 times per second (less data with 6 players, the clients
    // smooth the movements). A player who doesn't move is sent once per second
    // only.
    if (++positionTimer >= POSITION_SEND_DELAY)
    {
        positionTimer = 0;
        for (int i = 0; i < MaxPlayer; i++)
        {
            if (AllPlayers[i].Id == UNUSED || AllPlayers[i].IsDead)
                continue;

            formatPosition(text, sizeof(text), i);
            sentPositionAge[i] += POSITION_SEND_DELAY;
            if (strcmp(text, sentPositions[i]) == 0 && sentPositionAge[i] < POSITION_RESEND_FRAMES)
                continue;

            // Same size as text (96): the whole buffer is copied
            memcpy(sentPositions[i], text, sizeof(sentPositions[i]));
            sentPositionAge[i] = 0;
            sendToAllExcept(i, text);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
// Messages of the clients

/**
 * @brief Buy for a client (BUY request, index in the shop)
 *
 */
static void buyForPlayer(int playerIndex, int shopIndex)
{
    Player *player = &AllPlayers[playerIndex];
    bool infiniteMoney = allPartyModes[currentPartyMode].infiniteMoney;

    if (player->IsDead || shopIndex < 0)
        return;

    // Guns
    if (shopIndex < GunCount)
    {
        if (shopIndex == 0 || AllGuns[shopIndex].Price > player->Money)
            return;
        if (player->AllGunsInInventory[1] == shopIndex || player->AllGunsInInventory[2] == shopIndex)
            return;

        int slot = AllGuns[shopIndex].gunCategory == PISTOLS ? 1 : 2;
        player->AllGunsInInventory[slot] = shopIndex;
        ResetGunAmmo(playerIndex, slot);
        player->currentGunInInventory = slot;
        if (!infiniteMoney)
            reducePlayerMoney(playerIndex, AllGuns[shopIndex].Price);
        return;
    }

    // Grenades
    int grenadeIndex = shopIndex - GunCount;
    if (grenadeIndex >= 0 && grenadeIndex < shopGrenadeCount)
    {
        if (AllGrenades[grenadeIndex].Price > player->Money)
            return;

        // Same limits as buyGun() (player.c)
        int modeIndex = currentPartyMode < 3 ? currentPartyMode : 0;
        int maxQuantity = AllGrenades[grenadeIndex].maxQuantity[modeIndex];
        int ownedCount = 0;
        for (int i = InventoryGrenadeStartPosition; i < InventoryEquipementStartPosition; i++)
        {
            if (player->AllGunsInInventory[i] == shopIndex)
                ownedCount++;
        }
        bool canBuy = infiniteMoney ? ownedCount < maxQuantity : player->grenadeBought[grenadeIndex] < maxQuantity;

        for (int i = InventoryGrenadeStartPosition; i < InventoryEquipementStartPosition && canBuy; i++)
        {
            if (player->AllGunsInInventory[i] == EMPTY)
            {
                player->AllGunsInInventory[i] = shopIndex;
                if (!infiniteMoney)
                {
                    player->grenadeBought[grenadeIndex]++;
                    reducePlayerMoney(playerIndex, AllGrenades[grenadeIndex].Price);
                }
                break;
            }
        }
        return;
    }

    // Equipments
    int equipmentIndex = shopIndex - GunCount - shopGrenadeCount;
    if (equipmentIndex < 0 || equipmentIndex >= equipementCount || allEquipments[equipmentIndex].Price > player->Money)
        return;

    bool canBuy = true;
    switch (allEquipments[equipmentIndex].type)
    {
    case DEFUSER:
        canBuy = !player->haveDefuseKit && player->Team == COUNTERTERRORISTS;
        if (canBuy)
            player->haveDefuseKit = true;
        break;
    case KEVLAR_VEST:
        canBuy = player->armor != 100;
        if (canBuy)
            player->armor = 100;
        break;
    case KEVLAR_VEST_AND_HELMET:
        canBuy = !player->haveHeadset;
        if (canBuy)
        {
            player->armor = 100;
            player->haveHeadset = true;
        }
        break;
    default:
        canBuy = false;
        break;
    }

    if (canBuy && !infiniteMoney)
        reducePlayerMoney(playerIndex, allEquipments[equipmentIndex].Price);
}

/**
 * @brief Hit of a client: HIT;id;headshot;legshot;distance (4 values per hit)
 *
 */
static void treatHit(int shooterIndex, char **params, int paramCount)
{
    Player *shooter = &AllPlayers[shooterIndex];
    if (shooter->IsDead)
        return;

    int gunIndex = getPlayerCurrentGunIndex(shooter);
    if (gunIndex < 0 || gunIndex >= GunCount)
        return;

    for (int i = 1; i + 3 < paramCount; i += 4)
    {
        int hitIndex = getPlayerIndexById(atoi(params[i]));
        if (hitIndex == NO_PLAYER || hitIndex == shooterIndex || AllPlayers[hitIndex].IsDead)
            continue;

        shooter->IsHeadShot[0] = atoi(params[i + 1]) == 1;
        shooter->IsLegShot[0] = atoi(params[i + 2]) == 1;
        shooter->startGunIdRaycast = gunIndex;
        float distance = atof(params[i + 3]);
        makeHit(shooterIndex, hitIndex, distance, 0);
    }
}

/**
 * @brief A client planted the bomb (same code as the bots, main.c)
 *
 */
static void plantBombForPlayer(int playerIndex, float bombX, float bombY, float bombZ, int angle)
{
    Player *player = &AllPlayers[playerIndex];
    if (player->Team != TERRORISTS || !player->haveBomb || player->IsDead || BombPlanted || roundState != PLAYING)
        return;

    // The bomb must be near the player (bad data)
    if (fabsf(bombX - player->lerpDestination.x) > 2 || fabsf(bombZ - player->lerpDestination.z) > 2)
        return;

    BombPosition.x = bombX;
    BombPosition.y = bombY;
    BombPosition.z = bombZ;
    BombPosition.r = angle;

    BombPlanted = true;
    bombSet = true;
    bombPlantedAt = 0; // Only used by the bots
    bombPlanterIndex = playerIndex;

    PartyMinutes = allPartyModes[currentPartyMode].bombWaitingMinutesDuration;
    PartySeconds = allPartyModes[currentPartyMode].bombWaitingSecondsDuration;
    BombSeconds = allPartyModes[currentPartyMode].bombWaitingSecondsDuration;

    player->haveBomb = false;
    SetGunInInventoryForNonLocalPlayer(playerIndex, EMPTY, 8);
    NE_ModelSetCoord(Model[7], BombPosition.x, BombPosition.y, BombPosition.z);
    NE_ModelSetCoord(Model[10], BombPosition.x, BombPosition.y, BombPosition.z);
    Model[7]->rz = BombPosition.r;
    bombBipTimer = 120;
    SetBombDefuseZone(BombPosition.x, BombPosition.z, &bombDefuseZone);
    showPartyEventText(2);
}

/**
 * @brief A client defused the bomb (same code as the bots, main.c)
 *
 */
static void defuseBombForPlayer(int playerIndex)
{
    Player *player = &AllPlayers[playerIndex];
    if (player->Team != COUNTERTERRORISTS || player->IsDead || !BombPlanted || BombDefused ||
        (roundState != PLAYING && roundState != END_ROUND))
        return;

    // The player must be near the bomb (bad data)
    if (fabsf(BombPosition.x - player->lerpDestination.x) > 2 || fabsf(BombPosition.z - player->lerpDestination.z) > 2)
        return;

    BombDefused = true;
    bombDefuserIndex = playerIndex;
    currentDefuserIndex = NO_PLAYER;
    CounterScore++;

    if (LoseCountTerrorists > 0)
        LoseCountTerrorists--;

    if (LoseCountCounterTerrorists < 4)
        LoseCountCounterTerrorists++;

    addMoneyToTeam(allPartyModes[currentPartyMode].loseTheRoundMoney, TERRORISTS);
    addMoneyToTeam(allPartyModes[currentPartyMode].winTheRoundBombMoney, COUNTERTERRORISTS);

    showPartyEventText(3);
    setEndRound();
    CheckAfterRound();
}

/**
 * @brief A client threw a grenade: create it on the host and send it to the
 * other clients
 *
 */
static void throwGrenadeForPlayer(int playerIndex, const char *xDirection, const char *yDirection, const char *zDirection)
{
    Player *player = &AllPlayers[playerIndex];
    int gunIndex = getPlayerCurrentGunIndex(player);
    if (player->IsDead || gunIndex < GunCount || gunIndex >= GunCount + shopGrenadeCount)
        return;

    int grenadeType = AllGrenades[gunIndex - GunCount].type;

    // Position sent by the client just before the grenade
    int xPosition = floattof32(player->lerpDestination.x);
    int yPosition = floattof32(player->lerpDestination.y);
    int zPosition = floattof32(player->lerpDestination.z);

    PhysicalGrenade *grenade = CreateGrenade(grenadeType, player->Id);
    if (grenade != NULL)
        lanchGrenade(grenade, atof(xDirection), atof(yDirection), atof(zDirection), xPosition, yPosition, zPosition);

    // The grenade isn't in the inventory anymore (the client changes its gun)
    player->AllGunsInInventory[player->currentGunInInventory] = EMPTY;

    char text[128];
    snprintf(text, sizeof(text), "{GRENADE;%s;%s;%s;%d;%d;%d;%d}", xDirection, yDirection, zDirection,
             xPosition, yPosition, zPosition, grenadeType);
    sendToAllExcept(playerIndex, text);
}

/**
 * @brief Treat a message of a client (online client format: {type;values})
 *
 */
void localHostTreatClientMessage(int peerIndex, char **params, int paramCount)
{
    if (!hostGameRunning || paramCount == 0)
        return;

    int playerId = PEER_PLAYER_ID(peerIndex);
    int playerIndex = getPlayerIndexById(playerId);
    if (playerIndex == NO_PLAYER)
        return;

    Player *player = &AllPlayers[playerIndex];
    int type = atoi(params[0]);
    char text[128];

    switch (type)
    {
    case POS: // POS;x;y;z;angle;cameraAngle (x, y, z * 4096)
        if (paramCount >= 6 && ignorePositionFrames[playerIndex] == 0 && !player->IsDead)
        {
            snprintf(text, sizeof(text), "{POS;%d;%s;%s;%s;%s;%s}", playerId, params[1], params[2], params[3], params[4], params[5]);
            networkTreatText(text);
        }
        break;

    case SHOOT:
    {
        // Sound for the host (with the ammo) and the other clients
        snprintf(text, sizeof(text), "{SHOOT;%d;%d}", playerId, getPlayerCurrentGunIndex(player));
        networkTreatText(text);
        sendToAllExcept(playerIndex, text);
        break;
    }

    case HIT:
        treatHit(playerIndex, params, paramCount);
        break;

    case WALLHIT: // WALLHIT;x;y;z
        if (paramCount >= 4)
        {
            snprintf(text, sizeof(text), "{WALLHIT;%s;%s;%s}", params[1], params[2], params[3]);
            networkTreatText(text);
            sendToAllExcept(playerIndex, text);
        }
        break;

    case CURGUN: // CURGUN;slot
        if (paramCount >= 2)
        {
            int slot = atoi(params[1]);
            if (slot >= 0 && slot < inventoryCapacity && player->AllGunsInInventory[slot] != EMPTY)
            {
                player->currentGunInInventory = slot;
                // Already sent by the client: no CURGUN back to it
                sentPlayers[playerIndex].currentGun = slot;
                snprintf(text, sizeof(text), "{CURGUN;%d;%d}", playerId, slot);
                sendToAllExcept(playerIndex, text);
            }
        }
        break;

    case RELOADED:
        ReloadGun(playerIndex);
        break;

    case BUY: // BUY;shopIndex
        if (paramCount >= 2)
            buyForPlayer(playerIndex, atoi(params[1]));
        break;

    case BOMBPLACING: // Planting or defusing sound
        snprintf(text, sizeof(text), "{BOMBPLACING;%d}", playerId);
        networkTreatText(text);
        sendToAllExcept(playerIndex, text);
        break;

    case BOMBPLACE: // BOMBPLACE;x;y;z;angle (x, y, z * 4096)
        if (paramCount >= 5)
            plantBombForPlayer(playerIndex, atoi(params[1]) / 4096.0f, atoi(params[2]) / 4096.0f, atoi(params[3]) / 4096.0f, atoi(params[4]));
        break;

    case BOMBDEFUSE:
        defuseBombForPlayer(playerIndex);
        break;

    case GRENADE: // GRENADE;xDirection;yDirection;zDirection
        if (paramCount >= 4)
            throwGrenadeForPlayer(playerIndex, params[1], params[2], params[3]);
        break;

    case DROPGUN:
        dropCurrentGun(playerIndex);
        break;

    default:
        // TEAM (teams are given by the host), PING, VOTE: not used in local
        // multiplayer. GETBOMB: the host gives the bomb itself (the client
        // walks on it, see checkTakingBombZone())
        break;
    }
}

/**
 * @brief A client left the room: remove its player
 *
 */
void localHostOnPeerLeft(int peerIndex)
{
    if (!hostGameRunning)
        return;

    char text[32];
    snprintf(text, sizeof(text), "{LEAVE;%d}", PEER_PLAYER_ID(peerIndex));
    networkTreatText(text);
    sendToAllExcept(NO_PLAYER, text);

    int playerIndex = getPlayerIndexById(PEER_PLAYER_ID(peerIndex));
    if (playerIndex != NO_PLAYER)
        sentPlayers[playerIndex].known = false;
}

/**
 * @brief A client took a gun on the floor: pick up sound
 *
 */
void localHostOnGunPickup(int playerIndex)
{
    if (hostGameRunning)
        sendToPlayer(playerIndex, "{PICKUP}");
}
