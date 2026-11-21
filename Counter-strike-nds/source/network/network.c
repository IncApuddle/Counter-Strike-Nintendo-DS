// SPDX-License-Identifier: MIT
//
// Copyright (c) 2021-2022, Fewnity - Grégory Machefer
//
// This file is part of Counter Strike Nintendo DS Multiplayer Edition (CS:DS)

#include "../main.h"
#include "droppedGuns.h"
#include "localplay.h"
// ioctl() and FIONBIO (non-blocking sockets)
#if __has_include(<sys/ioctl.h>)
#include <sys/ioctl.h>
#endif
// inet_ntoa()
#if __has_include(<arpa/inet.h>)
#include <arpa/inet.h>
#endif
#include "sounds.h"
#include "movements.h"
#include "grenade.h"
#include "gun.h"
#include "ui.h"
#include "collisions.h"
#include "network.h"
#include "party.h"
#include "player.h"
#include "security.h"
#include "camera.h"

#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#if __has_include(<netinet/tcp.h>)
#include <netinet/tcp.h>
#endif
// Values used by lwIP (DSWifi in BlocksDS) if the headers don't define them
#ifndef IPPROTO_TCP
#define IPPROTO_TCP 6
#endif
#ifndef TCP_NODELAY
#define TCP_NODELAY 0x01
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif
#include "debug.h"
#include "map.h"
#include "draw3d.h"

// Current ping of the nintendo ds
int ping = 0;

//IP to which player connecting
char IpToGo[16] = "192.168.0.1";

// Socket
int my_socket = 0;

// Retain if the game needs to send a packet
bool SendPosition = false;
int SendPositionData = 3;
bool SendShoot = false;
bool SendLeave = false;
bool SendGrenade = false;
bool SendBombPlacing = false;
bool SendBombPlace = false;
bool SendBombDefused = false;
bool SendPing = false;
bool SendTeam = false;
bool WaitForTeamResponse = false;
bool SendBuyWeapon = false;
bool SendWallHit = false;
bool SendSelectedGun = false;
bool SendPlayerName = false;
bool SendVoteStartNow = false;

// Vote for the next mode during the warm up (online)
bool SendVoteMode = false;
int voteModeIndex = 0;         // 0 Competitive, 1 Casual, 2 Deathmatch, 3 GunGame
int modeVotes[MODE_VOTE_COUNT]; // Votes of all players (MODEVOTES)
int myModeVote = -1;            // Vote of the local player
bool SendKeyResponse = false;
bool SendGetDroppedBomb = false;
bool SendReloaded = false;

// Retain if the player is just connected
bool firstConnection = true;

// For security, we need to retain the number of frame showed
int frameCount = 0;
// Timeout timer for the connection
int timeOut = 0;

// Join type to send to the server
enum JoinType partyOption = JOIN_RANDOM_PARTY;

// Party code to send to the server or to receive from the server
char partyCode[PARTY_CODE_LENGTH];

// Is the party private?
bool isPrivate = false;

// Map change on the server: reconnect after the disconnection
bool autoReconnect = false;          // The server asked to reconnect (RECONNECT)
bool reconnectPending = false;       // Waiting before the reconnection
int reconnectDelay = 0;              // Frames before the reconnection
int reconnectOption = 0;             // JOIN_RANDOM_PARTY or JOIN_PRIVATE_PARTY (same code)

char Values[2048] = ""; // Store all the values received from the wifi and wainting for treatment

// Data waiting to be sent to the server. If the TCP buffer of the DS is full,
// send() only takes a part of the data. The old code lost the rest, cutting a
// packet in two, and the server then read broken data.
static char pendingSend[2048];
static int pendingSendLength = 0;

// Seconds without data from the server before disconnecting
#define NETWORK_TIMEOUT_SECONDS 7

static time_t lastDataTime = 0;

// UDP (player positions). With TCP, a packet lost on the wifi blocks all the
// following packets until it is sent again. Positions are sent over UDP: a lost
// one is replaced by the next one. Everything else stays on TCP. See
// UdpManager.cs in the server for the details.
static struct sockaddr_in serverAddress; // TCP address of the server
static int udpSocket = -1;
static int udpToken = 0;           // Given by the server, identifies us
static bool udpConfirmed = false;  // The server received our datagrams
static struct sockaddr_in udpServerAddress;
static int udpSendSequence = 0;
static int udpLastReceivedSequence = 0;
static int udpFramesSinceSend = 0;

// Send a datagram: "{T;token;sequence}" + packets ("" = keep alive)
static void udpSendDatagram(const char *packets)
{
    if (udpSocket < 0 || udpToken == 0)
        return;

    char buffer[256];
    int length = snprintf(buffer, sizeof(buffer), "{T;%d;%d}%s", udpToken, ++udpSendSequence, packets);
    if (length <= 0 || length >= (int)sizeof(buffer))
        return;

    sendto(udpSocket, buffer, length, 0, (struct sockaddr *)&udpServerAddress, sizeof(udpServerAddress));
    udpFramesSinceSend = 0;
}

// "UDP;port;token" received over TCP: open the UDP socket
static void udpOpen(int port, int token)
{
    if (udpSocket < 0)
    {
        udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udpSocket < 0)
        {
            netLog("UDP socket() failed (errno %d), positions use TCP", errno);
            return;
        }
        int blockmode = 1;
        ioctl(udpSocket, FIONBIO, &blockmode);
    }

    udpToken = token;
    udpConfirmed = false;
    udpSendSequence = 0;
    udpLastReceivedSequence = 0;
    udpServerAddress = serverAddress;
    udpServerAddress.sin_port = htons(port);
    netLog("UDP: port %d", port);

    // First datagram: the server learns our address
    udpSendDatagram("");
}

static void udpClose(void)
{
    if (udpSocket >= 0)
        closesocket(udpSocket);
    udpSocket = -1;
    udpToken = 0;
    udpConfirmed = false;
}

// Split "a;b;c" into arr. Returns the number of values.
static int splitPacketValues(char *packet, char arr[10][64])
{
    int count = 0;
    char *ptr = strtok(packet, ";");
    while (ptr != NULL && count < 10)
    {
        strncpy(arr[count], ptr, 63);
        arr[count][63] = '\0';
        count++;
        ptr = strtok(NULL, ";");
    }
    return count;
}

static void handlePositionPacket(char arr[][64]);

// Read the datagrams of the server: "{S;sequence}{POS;...}{POS;...}"
static void udpReceive(void)
{
    if (udpSocket < 0)
        return;

    // Up to 9 positions per datagram (one tick of the server)
    char buffer[1024];
    while (1)
    {
        struct sockaddr_in from;
        socklen_t fromLength = sizeof(from);
        int length = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&from, &fromLength);
        if (length <= 0)
            break;
        buffer[length] = '\0';

        // Only datagrams of the server
        if (from.sin_addr.s_addr != udpServerAddress.sin_addr.s_addr)
            continue;
        if (strncmp(buffer, "{S;", 3) != 0)
            continue;

        // Old datagram (arrived after a newer one): ignore it
        int sequence = atoi(buffer + 3);
        if (udpLastReceivedSequence != 0 && sequence <= udpLastReceivedSequence &&
            udpLastReceivedSequence - sequence < 100000)
            continue;
        udpLastReceivedSequence = sequence;

        char *position = strchr(buffer, '}');
        while (position != NULL)
        {
            char *start = strchr(position, '{');
            if (start == NULL)
                break;
            char *end = strchr(start + 1, '}');
            if (end == NULL)
                break;

            char packet[128];
            int packetLength = end - start - 1;
            if (packetLength > (int)sizeof(packet) - 1)
                packetLength = sizeof(packet) - 1;
            memcpy(packet, start + 1, packetLength);
            packet[packetLength] = '\0';

            char arr[10][64] = {0};
            if (splitPacketValues(packet, arr) >= 7 && strcmp(arr[0], "POS") == 0)
                handlePositionPacket(arr);

            position = end + 1;
        }
    }
}

void initNetwork(int option)
{
    // See Wifi_CheckInit to replace my_socket == 0 by !Wifi_CheckInit()
    // Call Wifi_InitDefault only once (by checking if the socket was already used), or the wifi will not work after that
    netLog("initNetwork: option %d, connection %d, socket %d", option, Connection, my_socket);
    // The wifi library is started without connection (INIT_ONLY) to be able
    // to use the local multiplayer mode too, then the game connects to the
    // access point saved in the DS settings
    if (!connectToAccessPoint())
    {
        netLog("Can't connect to the access point");
        Connection = UNSELECTED;
        initMainMenu();
    }
    else // Then connect to server
    {
        my_socket = socket(AF_INET, SOCK_STREAM, 0);
        netLog("socket() = %d (errno %d)", my_socket, errno);
        if (Connection == DEBUG_IP_1)
        {
            connectToServer(DEBUG_IP_1_STRING, false, my_socket, option);
        }
        else if (Connection == ONLINE_SERVER_IP)
        {
            connectToServer(IpToGo, false, my_socket, option);
        }
        else if (Connection == DEBUG_IP_2)
        {
            connectToServer(DEBUG_IP_2_STRING, false, my_socket, option);
        }
        else if (Connection == LOCAL)
        {
            connectToServer("", true, my_socket, option); // Local mode (Android phone with the android server version (unreleased))
        }

        Connection = UNSELECTED;
    }
}

/**
 * @brief Connect the game to a server, with an url.
 *
 *
 * @param url Url of the server
 * @param LocalMode Used to connect on a localserver on an android phone
 * @param my_socket socket
 * @param option (JOIN_RANDOM_PARTY, JOIN_PARTY_CODE, JOIN_PRIVATE_PARTY)
 */
void connectToServer(char *url, bool LocalMode, int my_socket, enum JoinType option)
{
    partyOption = option;

    // Tell the socket to connect to the IP address of the server, on port 6003
    // or 1080 for android phone
    struct sockaddr_in sain;
    memset(&sain, 0, sizeof(sain));
    sain.sin_family = AF_INET;
    if (!LocalMode)
        sain.sin_port = htons(SERVER_PORT);
    else
        sain.sin_port = htons(LOCAL_SERVER_PORT);

    bool addressFound = false;
    if (LocalMode)
    {
        // The server is on the phone: its address is the gateway address
        struct in_addr gateway;
        Wifi_GetIPInfo(&gateway, NULL, NULL, NULL);
        sain.sin_addr = gateway;
        addressFound = true;
    }
    else
    {
        // Find the IP address of the server (getaddrinfo() replaces the
        // deprecated gethostbyname())
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo *result = NULL;
        int error = getaddrinfo(url, NULL, &hints, &result);
        netLog("getaddrinfo(%s) = %d", url, error);
        if (error == 0 && result != NULL && result->ai_addr != NULL)
        {
            sain.sin_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
            addressFound = true;
        }
        if (result != NULL)
            freeaddrinfo(result);
    }

    if (!addressFound)
    {
        netLog("Can't find the server address");
        closesocket(my_socket);
        Connection = UNSELECTED;
        initMainMenu();
        return;
    }

    // Connect to the server
    int rc = connect(my_socket, (struct sockaddr *)&sain, sizeof(sain));
    netLog("connect(%s:%d) = %d (errno %d)", inet_ntoa(sain.sin_addr), ntohs(sain.sin_port), rc, errno);

    // Set socked in non block mode
    int blockmode = 1;
    rc = ioctl(my_socket, FIONBIO, &blockmode);
    netLog("ioctl(FIONBIO) = %d (errno %d)", rc, errno);

    // Send small packets immediately. With Nagle's algorithm (the default),
    // a small packet waits until the previous one is acknowledged.
    int noDelay = 1;
    rc = setsockopt(my_socket, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));
    netLog("setsockopt(TCP_NODELAY) = %d (errno %d)", rc, errno);

    pendingSendLength = 0;
    Values[0] = '\0';
    // Reset the timeout of the previous connection. If the previous game
    // ended while no data was received, the old time was kept and the new
    // connection was closed at once ("No data for 7 seconds"): it was only
    // possible to reconnect after restarting the game.
    timeOut = 0;
    lastDataTime = time(NULL);
    serverAddress = sain;
    udpClose();
    resetNetworkVariables();

    // Launch the read data service
    ReadServerData();
}

/**
 * @brief Reset network send values variables (call before connecting to a new server)
 *
 */
void resetNetworkVariables()
{
    SendShoot = false;
    SendLeave = false;
    SendGrenade = false;
    SendBombPlacing = false;
    SendBombPlace = false;
    SendBombDefused = false;
    SendPing = false;
    SendTeam = false;
    WaitForTeamResponse = false;
    SendBuyWeapon = false;
    SendWallHit = false;
    SendSelectedGun = false;
    SendPlayerName = false;
    SendVoteStartNow = false;
    SendKeyResponse = false;
    SendGetDroppedBomb = false;
    SendReloaded = false;
    SendDropGun = false;
}

static void logNetworkMemory(const char *reason);

/**
 * @brief Apply a POS packet (received over TCP or UDP)
 *
 * @param arr Packet values (arr[0] = "POS")
 */
static void handlePositionPacket(char arr[][64])
{
    // Get target player id
    int PlayerIdInt = intParse(arr[1]);

    float XFloat = floatParse(arr[2]);
    float YFloat = floatParse(arr[3]);
    float ZFloat = floatParse(arr[4]);
    // Change scale
    XFloat /= 4096.0;
    YFloat /= 4096.0;
    ZFloat /= 4096.0;

    int AngleInt = intParse(arr[5]);
    int CameraAngleInt = intParse(arr[6]);

    // If new position is for the local player
    if (localPlayer->Id == PlayerIdInt)
    {

        // Set new position and teleport player
        localPlayer->position.x = XFloat;
        localPlayer->position.y = YFloat;
        localPlayer->position.z = ZFloat;
        localPlayer->Angle = AngleInt;
        localPlayer->PlayerPhysic->xspeed = 0;
        localPlayer->PlayerPhysic->yspeed = 0;
        localPlayer->PlayerPhysic->zspeed = 0;
        NE_ModelSetCoord(localPlayer->PlayerModel, localPlayer->position.x, localPlayer->position.y, localPlayer->position.z);
        ForceUpdateLookRotation(localPlayer->cameraAngle);
    }
    else
    {
        // Find player with unique ID to update his informations
        for (int i = 1; i < MaxPlayer; i++)
            if (AllPlayers[i].Id == PlayerIdInt)
            {
                // If player has no position, teleport the player to new updated position (0 default position)
                if (AllPlayers[i].lerpDestination.x == 0)
                {
                    AllPlayers[i].position.x = XFloat;
                    AllPlayers[i].position.y = YFloat;
                    AllPlayers[i].position.z = ZFloat;
                    AllPlayers[i].Angle = AngleInt;
                }

                // Speed of the player between the last two positions
                // (used to predict where the player is now)
                Player *netPlayer = &AllPlayers[i];
                int framesSinceUpdate = onlineFrame - netPlayer->netLastFrame;
                float moveX = XFloat - netPlayer->netLastPos.x;
                float moveY = YFloat - netPlayer->netLastPos.y;
                float moveZ = ZFloat - netPlayer->netLastPos.z;
                if (netPlayer->lerpDestination.x == 0 || framesSinceUpdate <= 0 || framesSinceUpdate > 30 ||
                    fabsf(moveX) + fabsf(moveY) + fabsf(moveZ) > 4)
                {
                    // First position, old position or teleportation
                    netPlayer->netVelocity.x = 0;
                    netPlayer->netVelocity.y = 0;
                    netPlayer->netVelocity.z = 0;
                }
                else
                {
                    // Limit the speed (bad data, lag)
                    const float maxSpeed = 0.2f;
                    netPlayer->netVelocity.x = fmaxf(-maxSpeed, fminf(maxSpeed, moveX / framesSinceUpdate));
                    netPlayer->netVelocity.y = 0;
                    netPlayer->netVelocity.z = fmaxf(-maxSpeed, fminf(maxSpeed, moveZ / framesSinceUpdate));
                }
                netPlayer->netLastPos.x = XFloat;
                netPlayer->netLastPos.y = YFloat;
                netPlayer->netLastPos.z = ZFloat;
                netPlayer->netLastFrame = onlineFrame;

                // Set player destination
                AllPlayers[i].lerpDestination.x = XFloat;
                AllPlayers[i].lerpDestination.y = YFloat;
                AllPlayers[i].lerpDestination.z = ZFloat;

                AllPlayers[i].AngleDestination = AngleInt;

                // if (CurrentCameraPlayer == i)
                // CameraAngleY = CameraAngleInt;
                AllPlayers[i].cameraAngle = CameraAngleInt;
                break;
            }
    }
}

void treatData()
{
    int StartPosition, EndPosition;
    // printf("a:%s\n", Values);

    // If a complete packet is detected
    // (The old code subtracted pointers returned by strstr() even when they
    // were NULL. Check them before using them.)
    while (1)
    {
        char *startPtr = strchr(Values, '{');
        if (startPtr == NULL)
            break;
        char *endPtr = strchr(startPtr + 1, '}');
        if (endPtr == NULL)
            break;
        StartPosition = startPtr - Values + 1;
        EndPosition = endPtr - Values;

        char currentPacket[256] = "";
        int packetLength = EndPosition - StartPosition;
        if (packetLength > (int)sizeof(currentPacket) - 1)
            packetLength = sizeof(currentPacket) - 1;
        strncpy(currentPacket, Values + StartPosition, packetLength);

        //   Start spliting incoming data
        char *ptr = strtok(currentPacket, ";");
        int SplitCount = 0;
        // INVTORY has 11 values (id + 9 slots), more than the old 10: the
        // old code wrote past the end of the array.
        char arr[16][64] = {0};

        // Split data (packets with too many values or too long values would
        // overflow the array)
        while (ptr != NULL && SplitCount < 16)
        {
            strncpy(arr[SplitCount], ptr, sizeof(arr[0]) - 1);
            SplitCount++;
            ptr = strtok(NULL, ";");
        }

        netStage(arr[REQUEST_NAME_INDEX]);

        // Check packet info
        if (strcmp(arr[REQUEST_NAME_INDEX], "POS") == 0) // Player position update
        {
            handlePositionPacket(arr);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "WALLHIT") == 0) // Wall hit position update for hit flash at wall animation
        {
            int flashIndex = 0;
            for (int i = 0; i < FLASH_MODELS_COUNT; i++)
            {
                if (ShowWallHitFlash[i] == 0)
                {
                    flashIndex = i;
                    break;
                }
            }

            // Set animation timer
            ShowWallHitFlash[flashIndex] = 3;

            float XFloat = floatParse(arr[1]);
            float YFloat = floatParse(arr[2]);
            float ZFloat = floatParse(arr[3]);

            // Change scale
            XFloat /= 4096.0;
            YFloat /= 4096.0;
            ZFloat /= 4096.0;

            // Set effect position
            NE_ModelSetCoord(flashModels[flashIndex], XFloat, YFloat, ZFloat);

            // Get wall hit flash direction
            Vector2 Direction1D;
            Direction1D.y = YFloat - AllPlayers[CurrentCameraPlayer].position.y - CameraOffsetY + y;
            Direction1D.x = 1;
            normalize2D(&Direction1D);

            Vector3 Direction;
            Direction.x = XFloat - AllPlayers[CurrentCameraPlayer].position.x;
            Direction.y = YFloat - AllPlayers[CurrentCameraPlayer].position.y;
            Direction.z = ZFloat - AllPlayers[CurrentCameraPlayer].position.z;
            normalize(&Direction);

            // Set wall hit angle
            int FinalAngleY = atan2f(Direction.x, Direction.z) * 512.0 / (M_TWOPI) + 384;
            int FinalAngleZ = atan2f(Direction1D.y, 1) * 512.0 / (M_TWOPI) + 128;
            NE_ModelSetRot(flashModels[flashIndex], 0, FinalAngleY, FinalAngleZ);

            // Make 3D sound
            int Panning, Volume;
            Vector4 SoundPos;
            SoundPos.x = XFloat;
            SoundPos.y = YFloat;
            SoundPos.z = ZFloat;
            GetPanningByPosition(&Panning, &Volume, SoundPos, xWithoutYForAudio, zWithoutYForAudio, 0.15);
            Play3DSound(SFX_RIC, Volume, Panning, NULL);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SHOOT") == 0) // Shoot from a player
        {
            // Get player id
            int ParsedPlayerId = intParse(arr[1]);

            // Get gun id
            int ParsedGunId = intParse(arr[2]);

            // Make a sound
            int Panning, Volume;
            GetPanning(ParsedPlayerId, &Panning, &Volume, xWithoutYForAudio, zWithoutYForAudio, AllGuns[ParsedGunId].MaxSoundDistance);
            Player *player = NULL;
            int index = UNUSED;
            for (int i = 0; i < MaxPlayer; i++)
            {
                if (AllPlayers[i].Id == ParsedPlayerId)
                {
                    player = &AllPlayers[i];
                    index = i;
                    break;
                }
            }
            if (player != NULL)
            {
                if (player->currentGunInInventory == 1 || player->currentGunInInventory == 2)
                    player->AllAmmoMagazine[player->currentGunInInventory - 1].AmmoCount--;

                if (CurrentCameraPlayer == index)
                {
                    setGunRecoil(player);
                }

                if (ParsedGunId < GunCount)
                    Play3DSound(AllGuns[ParsedGunId].gunSound, Volume, Panning, player);
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "TimerA") == 0) // Timer changes
        {
            // Parse seconds and minutes texts to int
            PartyMinutes = intParse(arr[1]);
            PartySeconds = intParse(arr[2]);

            if (firstConnection)
            {
                BombSeconds = PartySeconds;
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "UDP") == 0) // UDP port and token for positions
        {
            udpOpen(intParse(arr[1]), intParse(arr[2]));
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "UDPOK") == 0) // The server receives our datagrams
        {
            udpConfirmed = true;
            netLog("UDP confirmed, positions use UDP");
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "PING") == 0) // Get ping request from server
        {
            // Parse ping text to int
            ping = intParse(arr[1]);
            SendPing = true;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "CURGUN") == 0) // Set current gun in inventory
        {
            int PlayerIdInt = intParse(arr[1]);

            // Find player to set health
            for (int i = 0; i < MaxPlayer; i++)
            {
                if (AllPlayers[i].Id == PlayerIdInt)
                {
                    int newGun = intParse(arr[2]);
                    // Gun changed by the server (dropped or picked up gun): stop the reload and the scope
                    if (i == 0 && AllPlayers[i].currentGunInInventory != newGun)
                    {
                        StopReloading(0);
                        DisableAim();
                    }
                    AllPlayers[i].currentGunInInventory = newGun;
                    // The slot can be empty until the inventory is received
                    if (i == CurrentCameraPlayer && getPlayerCurrentGunIndex(&AllPlayers[i]) != EMPTY)
                        UpdateGunTexture();

                    // GunGame: new gun with an empty magazine (the AMMO
                    // message can come before this one), reload it
                    Player *player = &AllPlayers[i];
                    if (i == 0 && currentPartyMode == 4 && !applyRules && (newGun == 1 || newGun == 2) &&
                        player->AllAmmoMagazine[newGun - 1].AmmoCount == 0 &&
                        player->AllAmmoMagazine[newGun - 1].TotalAmmoCount > 0)
                    {
                        startReloadGun(0);
                    }

                    break;
                }
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "HITSOUND") == 0) // Make a hit sound
        {
            int PlayerId = intParse(arr[2]);
            int HitType = intParse(arr[3]);

            int Panning, Volume;
            if (PlayerId != localPlayer->Id)
                GetPanning(PlayerId, &Panning, &Volume, xWithoutYForAudio, zWithoutYForAudio, 0.10);
            else
            {
                Panning = 128;
                Volume = 255;

                xOffset = (rand() % ScreenShakeAmount + ScreenShakeMinAmount) / 100.0;
                if (rand() % 2 == 0)
                    xOffset = -xOffset;

                yOffset = (rand() % ScreenShakeAmount + ScreenShakeMinAmount) / 100.0;
                if (rand() % 2 == 0)
                    yOffset = -yOffset;

                redHealthTextCounter = 62;
            }

            Player *player = NULL;
            for (int i = 0; i < MaxPlayer; i++)
            {
                if (AllPlayers[i].Id == PlayerId)
                {
                    player = &AllPlayers[i];
                    break;
                }
            }

            if (HitType == 0)
                Play3DSound(SFX_FLESH_IMPACT, Volume, Panning, player); // Check with kevlar
            else if (HitType == 1)
                Play3DSound(SFX_HEADSHOT1, Volume, Panning, player);
            else if (HitType == 2)
                Play3DSound(SFX_FLESH_IMPACT, Volume, Panning, player);
            else
                Play3DSound(SFX_KNIFE_HIT_PLAYER, Volume, Panning, player);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETID") == 0) // Add player
        {
            int ParsedId = intParse(arr[1]);

            // Spawn player
            AddNewPlayer(ParsedId, true, false);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETMAP") == 0) // Set the map
        {
            // Parse map id text to int
            int newMapId = intParse(arr[1]);

            netLog("SETMAP %s -> map %d", arr[1], newMapId);
            if (newMapId < 0 || newMapId >= MAP_COUNT)
                newMapId = DUST2;

            UnLoadMap(currentMap);
            currentMap = newMapId;
            LoadMap(currentMap);

            // Load the image of the map (map view, SELECT button). It was only
            // loaded for single player games, online the map was white. The
            // image is loaded as a 16 colors texture (MapImgToLoadFunc()): its
            // conversion needs less RAM (Mirage online, little free RAM).
            MapImgToLoad = currentMap;
            MapImgToLoadFunc();
            logNetworkMemory("SETMAP");
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETMODE") == 0) // Set the party mode
        {
            // Parse mode id text to int
            currentPartyMode = intParse(arr[1]);
            // New game (back to the warm up after the end of a game): the timer and the texts work again
            partyFinished = false;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "TEAM") == 0) // Change team
        {
            // Get player id
            int ParsedPlayerId = intParse(arr[1]);

            // Get Team value
            int ParsedIsCounter = intParse(arr[2]);

            // Find player and affect new value
            for (int i = 0; i < MaxPlayer; i++)
                if (AllPlayers[i].Id == ParsedPlayerId)
                {
                    AllPlayers[i].Team = ParsedIsCounter;
                    UpdatePlayerTexture(i);
                    break;
                }

            // Update screen if team screen is opened
            if (currentMenu == SCORE_BOARD)
            {
                UpdateBottomScreenFrameCount += 8;
            }
            else if (currentMenu == GAME)
            {
                initGameMenu();
            }

            // TODO check this
            if (localPlayer->Id == ParsedPlayerId)
            {
                if (roundState != TRAINING)
                    changeCameraPlayerView(false);

                if (ParsedIsCounter != -1)
                {
                    AllButtons[0].isHidden = false;
                }
                AllButtons[1].isHidden = false;
                AllButtons[2].isHidden = false;
                WaitForTeamResponse = false;
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "BOMBPLACE") == 0) // Bomb place update
        {
            // Convert chars into ints
            int XInt = intParse(arr[1]);
            int YInt = intParse(arr[2]);
            int ZInt = intParse(arr[3]);
            int AngleInt = intParse(arr[4]);
            int dropped = intParse(arr[5]);

            if (dropped == 1)
            {
                // Change values scale and set bomb position and angle value
                droppedBombPositionAndRotation.x = XInt / 4096.0;
                droppedBombPositionAndRotation.y = YInt / 4096.0;
                droppedBombPositionAndRotation.z = ZInt / 4096.0;
                droppedBombPositionAndRotation.r = AngleInt;

                // Set bomb 3D model position and angle value
                NE_ModelSetCoord(Model[7], droppedBombPositionAndRotation.x, droppedBombPositionAndRotation.y, droppedBombPositionAndRotation.z);
                // NE_ModelSetCoord(Model[10], droppedBombPositionAndRotation.x, droppedBombPositionAndRotation.y, droppedBombPositionAndRotation.z);
                Model[7]->rz = droppedBombPositionAndRotation.r;

                SetBombTakingZone(droppedBombPositionAndRotation.x, droppedBombPositionAndRotation.z, &bombDefuseZone); // Set zone for taking the bomb
                bombDropped = true;
            }
            else
            {
                // Change values scale and set bomb position and angle value
                BombPosition.x = XInt / 4096.0;
                BombPosition.y = YInt / 4096.0;
                BombPosition.z = ZInt / 4096.0;
                BombPosition.r = AngleInt;

                // Set bomb 3D model position and angle value
                NE_ModelSetCoord(Model[7], BombPosition.x, BombPosition.y, BombPosition.z);
                NE_ModelSetCoord(Model[10], BombPosition.x, BombPosition.y, BombPosition.z);
                Model[7]->rz = BombPosition.r;

                BombSeconds = allPartyModes[currentPartyMode].bombWaitingSecondsDuration;

                bombBipTimer = 120;
                BombPlanted = true;
                SetBombDefuseZone(BombPosition.x, BombPosition.z, &bombDefuseZone);
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "VOTERESULT") == 0) // Get the current vote values
        {
            int VoteTypeInt = intParse(arr[1]);

            if (VoteTypeInt == 0)
            {
                playerWantToStart = intParse(arr[2]);
                playerWantToStartLimite = intParse(arr[3]);
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "CONFIRM") == 0) // Confirm shop gun buy, or set player gun from server command
        {
            // Convert chars into ints
            int ConfirmTypeInt = intParse(arr[1]);
            int ConfirmArgumentInt = intParse(arr[2]);
            int ConfirmInventoryIndexInt = intParse(arr[3]);
            int ConfirmResultInt = intParse(arr[4]); // 0 Error, 1 Okay

            if (ConfirmTypeInt == 0) // Set gun in inventory
            {
                if (ConfirmResultInt == 0) // Error
                {
                }
                else if (ConfirmResultInt == 1) // Okay
                {
                    DisableAim();
                    SetGunInInventory(ConfirmArgumentInt, ConfirmInventoryIndexInt);
                    setSelectedGunInInventory(0, ConfirmInventoryIndexInt);

                    //Buy sound
                    PlayBasicSound(SFX_MOLOTOV_DETONATE);
                }
                else if (ConfirmResultInt == 2) // Not an error
                {
                    PlayBasicSound(SFX_MOLOTOV_DETONATE);
                }
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETMONEY") == 0) // Set player money
        {
            // Parse money text to int
            int ParsedMoney = intParse(arr[1]);
            setPlayerMoney(0, ParsedMoney);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SCORE") == 0) // Party score changes
        {
            // Parse scores texts to int
            CounterScore = intParse(arr[1]);
            TerroristsScore = intParse(arr[2]);

            // Update screen if team screen is opened
            if (currentMenu == SCORE_BOARD)
                UpdateBottomScreenFrameCount += 8;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SCRBOARD") == 0) // Score board score changes
        {
            // Get player id
            int ParsedPlayerId = intParse(arr[1]);

            // Find player with unique ID to update his informations
            for (int i = 0; i < MaxPlayer; i++)
                if (AllPlayers[i].Id == ParsedPlayerId)
                {
                    AllPlayers[i].KillCount = intParse(arr[2]);
                    AllPlayers[i].DeathCount = intParse(arr[3]);
                    break;
                }

            // Update screen if team screen is opened
            if (currentMenu == SCORE_BOARD)
                UpdateBottomScreenFrameCount += 8;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETSHOPZONE") == 0) // Update the shop zone
        {
            shopDisableTimer = SHOP_DISABLE_TIMER;
            setShopZone(localPlayer);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "PartyRound") == 0) // Round state changes
        {
            // Parse text to int
            int tempRoundState = intParse(arr[1]);

            roundState = tempRoundState;

            // Reset some values on new round
            if (roundState == WAIT_START)
            {
                onNewRoundStart();
                NE_SpriteVisible(TopScreenSprites[0], true);

                StopReloading(0);
                ResetGunsAmmo(0);
                if (CurrentCameraPlayer != 0)
                {
                    SetCurrentCameraPlayer(0);
                }
                for (int i = 0; i < MaxPlayer; i++)
                    AllPlayers[i].IsDead = false;

                deathCameraAnimation = 0;
                deathCameraYOffset = 0;
                redHealthTextCounter = 0;
            }
            else if (roundState == PLAYING)
            {
                // Play the start round clip
                PlayBasicSound(SFX_ZOOM);
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "LEAVE") == 0) // When a player leave
        {
            // Get player id
            int ParsedPlayerId = intParse(arr[1]);

            // Find player
            for (int i = 0; i < MaxPlayer; i++)
                if (AllPlayers[i].Id == ParsedPlayerId)
                {
                    AllPlayers[i].Id = UNUSED;
                    PlayerCount--;
                    NE_ModelDelete(AllPlayers[i].PlayerModel);
                    if (i == 0)
                    {
                        CS_PhysicsDelete(AllPlayers[i].PlayerPhysic);
                    }
                    else
                    {
                        showDisconnectedText(i);
                    }
                    AllPlayers[i].KillCount = 0;
                    AllPlayers[i].Team = SPECTATOR;
                    AllPlayers[i].DeathCount = 0;
                    if (currentMenu == SCORE_BOARD)
                        initScoreMenu();

                    if (CurrentCameraPlayer == i)
                    {
                        SetCurrentCameraPlayer(0);
                    }

                    break;
                }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETHEALTH") == 0) // Set player health
        {
            int PlayerIdInt = intParse(arr[1]);

            // Find player to set health
            for (int i = 0; i < MaxPlayer; i++)
            {
                if (AllPlayers[i].Id == PlayerIdInt)
                {
                    int newHealth = intParse(arr[2]);
                    setPlayerHealth(i, newHealth);
                    AllPlayers[i].armor = intParse(arr[3]);

                    int haveHeadset = intParse(arr[4]);
                    AllPlayers[i].haveHeadset = haveHeadset == 1 ? true : false;
                    break;
                }
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETBOMB") == 0) // Set/remove the bomb of a player
        {
            // Get player id
            int ParsedPlayerId = intParse(arr[1]);

            // Get gun id
            int haveBomb = intParse(arr[2]);

            for (int i = 0; i < MaxPlayer; i++)
            {
                if (AllPlayers[i].Id == ParsedPlayerId && haveBomb == 1)
                {
                    AllPlayers[i].haveBomb = true;
                    bombDropped = false;
                    SetGunInInventoryForNonLocalPlayer(i, 28, 8);
                }
                else
                {
                    AllPlayers[i].haveBomb = false;
                    SetGunInInventoryForNonLocalPlayer(i, EMPTY, 8);
                }
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "RELOADED") == 0) // Reload the gun of a player
        {
            // Get player id
            int ParsedPlayerId = intParse(arr[1]);

            int Reset = intParse(arr[2]);

            // The message has the player id, the functions need the player index
            int playerIndex = -1;
            for (int i = 0; i < MaxPlayer; i++)
            {
                if (AllPlayers[i].Id == ParsedPlayerId)
                {
                    playerIndex = i;
                    break;
                }
            }

            if (playerIndex == -1)
            {
                // Unknown player
            }
            else if (Reset == 0)
            {
                ReloadGun(playerIndex);
            }
            else if (Reset == 1)
            {
                ResetGunsAmmo(playerIndex);
            }
            else if (Reset == 2)
            {
                int GunId = intParse(arr[4]);
                ResetGunAmmo(playerIndex, GunId);
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETNAME") == 0) // Get a player name
        {
            // Get player id
            int ParsedPlayerId = intParse(arr[1]);

            for (int i = 0; i < MaxPlayer; i++)
            {
                if (AllPlayers[i].Id == ParsedPlayerId)
                {
                    strcpy(AllPlayers[i].name, arr[2]);
                    if (i != 0 && !firstConnection)
                        showConnectedText(i);
                    break;
                }
            }

            // Update screen if team screen is opened
            if (currentMenu == 2)
                UpdateBottomScreenFrameCount += 8;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "SETCODE") == 0) // Get the party code
        {
            strcpy(partyCode, arr[1]);
            isPrivate = true;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "ERROR") == 0) // Show text from server
        {
            int errorId = intParse(arr[1]);
            if (errorId == Ban)
            {
                strcpy(errorText, "You are banned!");
            }
            else if (errorId == WrongVersion)
            {
                strcpy(errorText, "You are using an old version of the game, please update your game.");
            }
            else if (errorId == MacAddressMissing)
            {
                strcpy(errorText, "Your Nintendo DS doesn't have a MAC address.");
            }
            else if (errorId == WrongSecurityKey)
            {
                strcpy(errorText, "You are using a modified game.");
            }
            else if (errorId == ServerFull)
            {
                strcpy(errorText, "The server is currently full, please try again later.");
            }
            else if (errorId == ServerStopped)
            {
                strcpy(errorText, "The server is currently under maintenance.");
            }
            else if (errorId == SaveCorrupted)
            {
                strcpy(errorText, "Your save is corrupted.");
            }
            else if (errorId == IncorrectCode)
            {
                strcpy(errorText, "The code is incorrect.");
            }
            else if (errorId == KickTeamKill)
            {
                strcpy(errorText, "You have been kicked (Team kill).");
            }
            else if (errorId == KickSpeedHack)
            {
                strcpy(errorText, "You have been kicked (game speed-up is not allowed).");
            }
            else
            {
                strcpy(errorText, "An error has occurred!");
            }

            initOnlineErrorMenu();
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "TEXT") == 0) // Show text from server
        {
            int ParsedTextId = intParse(arr[1]);

            showPartyEventText(ParsedTextId);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "TEXTPLAYER") == 0) // Show text about a player from server
        {
            int ParsedPlayerId = intParse(arr[1]);

            int KillerIdInt = intParse(arr[2]);
            // int TextIdInt = intParse(arr[3]); unused

            int killerPlayerIndex = NO_PLAYER;
            int killedPlayerIndex = NO_PLAYER;

            if (KillerIdInt != NO_PLAYER)
            {
                // Find player with unique ID to update his informations
                for (int i = 0; i < MaxPlayer; i++)
                {
                    if (AllPlayers[i].Id == KillerIdInt)
                    {
                        killerPlayerIndex = i;
                        break;
                    }
                }
            }

            if (ParsedPlayerId != NO_PLAYER)
            {
                for (int i = 0; i < MaxPlayer; i++)
                {
                    if (AllPlayers[i].Id == ParsedPlayerId)
                    {
                        killedPlayerIndex = i;
                        break;
                    }
                }
            }

            if (killerPlayerIndex != NO_PLAYER && killerPlayerIndex != NO_PLAYER)
                showKillText(killerPlayerIndex, killedPlayerIndex);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "BOMBPLACING") == 0) // Bomb placing sound
        {
            int ParsedPlayerId = intParse(arr[1]);

            // Heard from farther than the other sounds (about 32 units)
            int Panning, Volume;
            GetPanning(ParsedPlayerId, &Panning, &Volume, xWithoutYForAudio, zWithoutYForAudio, 0.25);

            Play3DSound(SFX_BOMBPLANTING, Volume, Panning, GetPlayer(ParsedPlayerId));
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "BOMBDEFUSE") == 0) // Bomb defuse sound
        {
            BombDefused = true;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "ADDRANGE") == 0) // Add multiples non local players
        {
            for (int i = 1; i < SplitCount; i++)
            {
                int FoundId = intParse(arr[i]);
                AddNewPlayer(FoundId, false, false);
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "ENDGAME") == 0) // End the party
        {
            partyFinished = true;
            // Map change: no final score, the game reconnects
            if (!autoReconnect)
                initFinalScoreMenu();
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "MODEVOTES") == 0) // Votes for the next mode
        {
            for (int i = 0; i < MODE_VOTE_COUNT && i + 1 < SplitCount; i++)
                modeVotes[i] = intParse(arr[i + 1]);

            // New warm up: the local vote is forgotten when all votes are 0
            int total = 0;
            for (int i = 0; i < MODE_VOTE_COUNT; i++)
                total += modeVotes[i];
            if (total == 0)
                myModeVote = -1;

            if (currentMenu == VOTE_MODE)
                UpdateBottomScreenFrameCount += 8;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "GAMEOVER") == 0) // End of the game, back to the warm up in 10 seconds
        {
            int winnerId = intParse(arr[1]);
            int winnerTeam = intParse(arr[2]);

            if (winnerId != -1)
            {
                // Deathmatch / GunGame: a player wins
                Player *winner = GetPlayer(winnerId);
                if (winner != NULL)
                {
                    snprintf(gameEventNotificationText, sizeof(gameEventNotificationText), "%s wins!", winner->name);
                    textToShowTimer = 600;
                }
            }
            else if (winnerTeam == TERRORISTS)
            {
                showPartyEventText(TERRORISTS_WIN);
            }
            else if (winnerTeam == COUNTERTERRORISTS)
            {
                showPartyEventText(COUNTER_TERRORISTS_WIN);
            }
            else
            {
                strncpy(gameEventNotificationText, "Draw", sizeof(gameEventNotificationText));
                textToShowTimer = 600;
            }

            // Show the scores
            initScoreMenu();
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "RECONNECT") == 0) // Map change: reconnect after the disconnection
        {
            autoReconnect = true;
            // Private party: join the new party with the same code
            reconnectOption = isPrivate ? JOIN_PRIVATE_PARTY : JOIN_RANDOM_PARTY;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "CHANGMAP") == 0) // End the party
        {
            PlayBasicSound(SFX_GRENADE_HIT1); //changing map announcement
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "ENDUPDATE") == 0) // End the update party sequence
        {
            firstConnection = false;
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "DROPGUN") == 0) // A gun is on the floor
        {
            // DROPGUN;netId;gunId;x;y;z (positions * 4096)
            if (SplitCount >= 6)
            {
                int netId = intParse(arr[1]);
                int gunId = intParse(arr[2]);
                float x = intParse(arr[3]) / 4096.0;
                float y = intParse(arr[4]) / 4096.0;
                float z = intParse(arr[5]) / 4096.0;

                // The gun can be sent again (join during a round)
                if (netId >= 0)
                    removeDroppedGunByNetId(netId);
                addDroppedGun(gunId, x, y, z, 0, 0, NO_PLAYER, netId);
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "PICKUP") == 0) // We took a gun on the floor
        {
            // Same sound as in single player (and the shop)
            PlayBasicSound(SFX_MOLOTOV_DETONATE);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "REMOVEGUN") == 0) // A gun was taken (-1: remove all guns)
        {
            removeDroppedGunByNetId(intParse(arr[1]));
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "INVTORY") == 0) // Get the inventory of a player
        {
            int ParsedPlayerId = intParse(arr[1]);

            int FoundPlayer = UNUSED;
            for (int i = 0; i < MaxPlayer; i++)
            {
                if (AllPlayers[i].Id == ParsedPlayerId)
                {
                    FoundPlayer = i;
                    // Set all the slots first: checking the gun in the hands after each
                    // slot would see a slot not set yet as empty (and change the gun)
                    for (int i2 = 2; i2 < SplitCount && i2 - 2 < inventoryCapacity; i2++)
                    {
                        AllPlayers[i].AllGunsInInventory[i2 - 2] = intParse(arr[i2]);
                        ResetGunAmmo(i, i2 - 2);
                    }

                    // No gun in the hands (gun removed): take the best gun
                    if (getPlayerCurrentGunIndex(&AllPlayers[i]) == EMPTY)
                    {
                        int newSlot = 0;
                        if (AllPlayers[i].AllGunsInInventory[2] != EMPTY)
                            newSlot = 2;
                        else if (AllPlayers[i].AllGunsInInventory[1] != EMPTY)
                            newSlot = 1;

                        if (i == 0)
                        {
                            StopReloading(0);
                            DisableAim();
                            setSelectedGunInInventory(0, newSlot);
                        }
                        else
                        {
                            AllPlayers[i].currentGunInInventory = newSlot;
                        }
                    }
                    break;
                }
            }
            // If the current player camera is looked by the local player, update the gun texture
            if (FoundPlayer == CurrentCameraPlayer)
            {
                UpdateGunTexture();
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "AMMO") == 0) // Get the ammo count of a player
        {
            int ParsedPlayerId = intParse(arr[1]);
            int FoundPlayer = UNUSED;
            for (int playerIndex = 0; playerIndex < MaxPlayer; playerIndex++)
            {
                if (AllPlayers[playerIndex].Id == ParsedPlayerId)
                {
                    FoundPlayer = playerIndex;
                    for (int i2 = 0; i2 < 2; i2++)
                    {
                        int AmmoCountInt = intParse(arr[2 + i2 * 2]);
                        int TotalAmmoCountInt = intParse(arr[2 + i2 * 2 + 1]);

                        AllPlayers[FoundPlayer].AllAmmoMagazine[i2].AmmoCount = AmmoCountInt;
                        AllPlayers[FoundPlayer].AllAmmoMagazine[i2].TotalAmmoCount = TotalAmmoCountInt;
                    }

                    // GunGame: new gun with an empty magazine, reload before shooting
                    Player *player = &AllPlayers[FoundPlayer];
                    if (FoundPlayer == 0 && currentPartyMode == 4 && !applyRules &&
                        (player->currentGunInInventory == 1 || player->currentGunInInventory == 2) &&
                        player->AllAmmoMagazine[player->currentGunInInventory - 1].AmmoCount == 0 &&
                        player->AllAmmoMagazine[player->currentGunInInventory - 1].TotalAmmoCount > 0)
                    {
                        startReloadGun(0);
                    }
                    break;
                }
            }
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "KEY") == 0) // Get the key from the server
        {
            SendKeyResponse = true;
            serverKey = intParse(arr[1]);
        }
        else if (strcmp(arr[REQUEST_NAME_INDEX], "GRENADE") == 0) // Spawn a player's grenade
        {
            // Convert chars into ints
            float XDirection = floatParse(arr[1]);
            float YDirection = floatParse(arr[2]);
            float ZDirection = floatParse(arr[3]);
            int XInt = intParse(arr[4]);
            int YInt = intParse(arr[5]);
            int ZInt = intParse(arr[6]);
            int GrenadeType = intParse(arr[7]);

            PhysicalGrenade *newGrenade = CreateGrenade(GrenadeType, 0);
            if (newGrenade != NULL)
            {
                lanchGrenade(newGrenade, XDirection, YDirection, ZDirection, XInt, YInt, ZInt);
            }
        }

        // Remove the packet from "Values" (move the rest of the data to the
        // start, the old code cleared and copied a 1 KB buffer per packet)
        char *rest = Values + EndPosition + 1;
        memmove(Values, rest, strlen(rest) + 1);
    }
}

/**
 * @brief Read data from the game server
 *
 */

// Write free RAM and VRAM to the network log
static void logNetworkMemory(const char *reason)
{
    netLog("mem (%s): RAM %d, textures %d, palettes %d", reason,
           (int)GetFreeSize(), NE_TextureFreeMem(), NE_PaletteFreeMem());
}

void ReadServerData()
{
    // All temp variable for incoming data
    int recvd_len = 0;
    char incoming_buffer[512];

    bool connectionClosed = false;

    while (1)
    {
        // Read everything the server sent since the last frame. The old code
        // read at most 63 bytes per frame (about 3.7 KB/s at 60 FPS), so with
        // several players the data piled up and the game fell behind.
        int receivedThisFrame = 0;
        while (1)
        {
            recvd_len = recv(my_socket, incoming_buffer, sizeof(incoming_buffer) - 1, 0);

            if (recvd_len > 0)
            {
                {
                    char logText[48];
                    int n = recvd_len < 40 ? recvd_len : 40;
                    memcpy(logText, incoming_buffer, n);
                    logText[n] = 0;
                    static int recvLogCount = 0;
                    if (recvLogCount < 40)
                    {
                        recvLogCount++;
                        netLog("recv %d: %s", recvd_len, logText);
                    }
                }
                incoming_buffer[recvd_len] = 0; // null-terminate
                // If a packet never ends (bad data), drop the buffer instead
                // of writing past its end.
                if (strlen(Values) + recvd_len >= sizeof(Values))
                {
                    netLog("Receive buffer full, data dropped");
                    Values[0] = '\0';
                }
                strncat(Values, incoming_buffer, recvd_len);
                netStage("treatData");
                treatData();

                receivedThisFrame += recvd_len;
                // Don't spend the whole frame here if the server sends a lot
                if (receivedThisFrame >= 4096)
                    break;
            }
            else if (recvd_len == 0)
            {
                // The server closed the connection
                connectionClosed = true;
                break;
            }
            else
            {
                // -1 with EAGAIN/EWOULDBLOCK = no more data for now. Any
                // other error means that the connection is broken.
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    netLog("recv error %d", errno);
                    connectionClosed = true;
                }
                break;
            }
        }

        if (connectionClosed)
            break;

        // Positions of the other players (UDP)
        udpReceive();

        if (receivedThisFrame > 0)
        {
            timeOut = 0;
        }
        else
        {
            // The server sends something every second (timer, ping). The
            // old code counted 400 loops without data, but with BlocksDS a
            // loop can be shorter than a frame (Nitro Engine doesn't wait for
            // VBlank if the frame took too long), so use the real time.
            if (timeOut == 0)
                lastDataTime = time(NULL);
            timeOut++;
            if (time(NULL) - lastDataTime >= NETWORK_TIMEOUT_SECONDS)
            {
                netLog("No data for %d seconds (%d loops): disconnect", NETWORK_TIMEOUT_SECONDS, timeOut);
                break;
            }
        }

        if (localPlayer->Id != UNUSED)
        {
            // Play game code
            netStage("GameLoop");
            GameLoop();
        }

        // In BlocksDS, the TCP/IP stack (lwIP) runs in its own thread. It
        // only gets CPU time when the main thread yields, and the game only
        // did that in swiWaitForVBlank() (which Nitro Engine skips when the
        // frame takes too long). Without this, received packets are never
        // processed and recv() never gets the data sent by the server.
        netStage("yield 1");
        cothread_yield();

        static int memLogTimer = 0;
        if (++memLogTimer >= 600)
        {
            memLogTimer = 0;
            logNetworkMemory("timer");
        }

        static int loopLogCount = 0;
        if (loopLogCount < 3)
            netLog("loop %d: UpdateEngine", loopLogCount);
        netStage("UpdateEngine");
        UpdateEngine();
        if (loopLogCount < 3)
            netLog("loop %d: sendDataToServer", loopLogCount);
        netStage("sendDataToServer");
        sendDataToServer();
        netStage("yield");
        cothread_yield();
        netStage("recv");
        if (loopLogCount < 3)
            netLog("loop %d: done", loopLogCount);
        loopLogCount++;
    }
    netLog("Connection closed (recv %d, errno %d, closed %d)", recvd_len, errno, connectionClosed);
    shutdown(my_socket, 0); // good practice to shutdown the socket.
    closesocket(my_socket);
    udpClose();
    Connection = UNSELECTED;
    if (autoReconnect)
    {
        // Map change: show a message, reconnect in a few seconds (main loop)
        autoReconnect = false;
        reconnectPending = true;
        reconnectDelay = 240; // 4 seconds, time for the server to change the map
        // Same cleaning as a return to the main menu (removes the players
        // of the old party, or the new party can't be joined)
        initMainMenu();
        strcpy(errorText, "The server is changing the map. Reconnecting...");
        initOnlineErrorMenu();
    }
    else if (!partyFinished && currentMenu != ONLINE_ERROR)
    {
        initMainMenu();
    }
    partyFinished = false;
    isPrivate = false;
    firstConnection = true;
    OnPartyQuit();
}

/**
 * @brief Send data to the server
 *
 */

static void queueDataToServer(const char *data, int length)
{
    if (pendingSendLength + length > (int)sizeof(pendingSend))
    {
        // The server doesn't read our data anymore. Drop the oldest data,
        // the connection will time out if it doesn't come back.
        netLog("Send buffer full, %d bytes dropped", pendingSendLength);
        pendingSendLength = 0;
        if (length > (int)sizeof(pendingSend))
            return;
    }
    memcpy(pendingSend + pendingSendLength, data, length);
    pendingSendLength += length;
}

static void flushDataToServer(void)
{
    if (pendingSendLength == 0)
        return;

    netStage("send");
    int sent = send(my_socket, pendingSend, pendingSendLength, 0);

    static int sendLogCount = 0;
    if (sendLogCount < 20 || sent < 0)
    {
        sendLogCount++;
        char logText[48];
        int n = pendingSendLength < 40 ? pendingSendLength : 40;
        memcpy(logText, pendingSend, n);
        logText[n] = 0;
        netLog("send %d -> %d (errno %d): %s", pendingSendLength, sent, errno, logText);
    }

    if (sent <= 0)
        return; // Try again in the next frame

    // Keep what wasn't sent for the next frame
    pendingSendLength -= sent;
    if (pendingSendLength > 0)
        memmove(pendingSend, pendingSend + sent, pendingSendLength);
}

void sendDataToServer()
{
    // Check if position need to be updated for other player
    if (localPlayer->position.x != OldxPos || localPlayer->position.y != OldyPos || localPlayer->position.z != OldzPos)
    {
        OldxPos = localPlayer->position.x;
        OldyPos = localPlayer->position.y;
        OldzPos = localPlayer->position.z;
        SendPosition = true;
    }

    char InfoToSend[1024] = "";

    // Send shoot data for sound/animation for clients
    if (SendLeave)
    {
        sprintf(InfoToSend + strlen(InfoToSend), "{%d}", LEAVE);
        SendLeave = false;
    }

    // Send position one in two
    if (SendPositionData > 0)
        SendPositionData--;

    // When the player stops, send the position one more time: the other
    // players then see a speed of 0 and stop predicting a movement.
    static bool sentStoppedPosition = true;
    if (SendPosition)
        sentStoppedPosition = false;
    else if (!sentStoppedPosition && SendPositionData == 0)
    {
        SendPosition = true;
        sentStoppedPosition = true;
    }

    // If local player need to be updated for other player (59 bytes).
    // Not added if old data is still waiting to be sent (bad connection):
    // the position is sent later, so only the newest one is sent.
    if (localPlayer->Id != -1 && SendPosition && SendPositionData == 0 && (udpConfirmed || pendingSendLength < 256))
    {
        char positionText[96];
        snprintf(positionText, sizeof(positionText), "{%d;%d;%d;%d;%d;%0.0f}", POS, localPlayer->PlayerModel->x, localPlayer->PlayerModel->y, localPlayer->PlayerModel->z, (int)localPlayer->Angle, localPlayer->cameraAngle);
        if (udpConfirmed)
            udpSendDatagram(positionText);
        else
            strcat(InfoToSend, positionText);

        SendPosition = false;
        // Every 2 frames (30 times per second): decremented before the test
        SendPositionData = 2;
    }

    // Send grenade launch
    if (SendGrenade)
    {
        sprintf(InfoToSend + strlen(InfoToSend), "{%d;%f;%f;%f}", GRENADE, x, y, z);
        SendGrenade = false;
    }

    // Send bomb placement data
    if (SendBombPlace)
    {
        SendBombPlace = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d;%d;%d;%d;%d}", BOMBPLACE, (int)(BombPosition.x * 4096.0), (int)(BombPosition.y * 4096.0), (int)(BombPosition.z * 4096.0), BombPosition.r);
    }

    // Send bomb defuse data
    if (SendBombDefused)
    {
        SendBombDefused = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d}", BOMBDEFUSE);
    }

    // Send bomb planting data
    if (SendBombPlacing)
    {
        SendBombPlacing = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d}", BOMBPLACING);
    }

    // Send team data
    if (SendTeam)
    {
        SendTeam = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d;%d}", TEAM, tempTeam);
        tempTeam = -1;
    }

    // Send ping reply
    if (SendPing)
    {
        SendPing = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d}", PING);
    }

    // Send buy weapon request data
    if (SendBuyWeapon)
    {
        SendBuyWeapon = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d;%d}", BUY, GetSelectedGunShop());
    }

    // Send gun reloaded data
    if (SendReloaded)
    {
        SendReloaded = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d}", RELOADED);
    }

    // Send selected gun
    if (SendSelectedGun)
    {
        SendSelectedGun = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d;%d}", CURGUN, localPlayer->currentGunInInventory);
    }

    // Send shoot data for sound/animation for clients
    if (SendShoot)
    {
        sprintf(InfoToSend + strlen(InfoToSend), "{%d}", SHOOT);
        SendShoot = false;
    }

    // Check if the game needs to send hitted clients
    bool sendHitClient = false;
    for (int i = 0; i < FLASH_MODELS_COUNT; i++)
    {
        if (hittedClient[i] != NO_PLAYER)
        {
            sendHitClient = true;
            break;
        }
    }

    // Send hit data
    if (sendHitClient)
    {
        sprintf(InfoToSend + strlen(InfoToSend), "{%d", HIT);
        for (int i = 0; i < FLASH_MODELS_COUNT; i++)
        {
            if (hittedClient[i] != -1)
            {
                sprintf(InfoToSend + strlen(InfoToSend), ";%d;%d;%d;%f", AllPlayers[hittedClient[i]].Id, localPlayer->IsHeadShot[i], localPlayer->IsLegShot[i], hitDistance[i]);
                hittedClient[i] = NO_PLAYER;
            }
        }
        sprintf(InfoToSend + strlen(InfoToSend), "}");
    }

    // Send vote
    if (SendVoteStartNow)
    {
        SendVoteStartNow = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d;0}", VOTE);
    }

    // Send the vote for the next mode
    if (SendVoteMode)
    {
        SendVoteMode = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d;1;%d}", VOTE, voteModeIndex);
    }

    // Send wall hit data
    if (SendWallHit)
    {
        SendWallHit = false;
        for (int i = 0; i < getPlayerCurrentGun(localPlayer).bulletCountPerShoot; i++)
        {
            sprintf(InfoToSend + strlen(InfoToSend), "{%d;%d;%d;%d}", WALLHIT, WallHitXPos[i], WallHitYPos[i], WallHitZPos[i]);
        }
    }

    // Send local player name
    if (SendPlayerName)
    {
        SendPlayerName = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d;%s}", SETNAME, localPlayer->name);
    }

    // Send security key response
    if (SendKeyResponse)
    {
        SendKeyResponse = false;

        u8 macAddress[6];
        Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, macAddress);

        sprintf(InfoToSend + strlen(InfoToSend), "{%d;%d;%02X%02X%02X%02X%02X%02X;%s;%s}", KEY, getKeyResponse(serverKey), macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5], localPlayer->name, GAME_VERSION);

        if (partyOption == JOIN_PRIVATE_PARTY)
            sprintf(InfoToSend + strlen(InfoToSend), "{%d;%d;%s}", PARTY, partyOption, partyCode);
        else
            sprintf(InfoToSend + strlen(InfoToSend), "{%d;%d}", PARTY, partyOption);
    }

    // Send get bomb dropped event
    if (SendGetDroppedBomb)
    {
        SendGetDroppedBomb = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d}", GETBOMB);
    }

    // Drop the gun in the hands
    if (SendDropGun)
    {
        SendDropGun = false;
        sprintf(InfoToSend + strlen(InfoToSend), "{%d}", DROPGUN);
    }

    // Local multiplayer (NiFi): send to the host DS
    if (Connection == LOCAL_GAME)
    {
        // Messages one by one (a packet never cuts a message)
        if (InfoToSend[0] != '\0')
            localSendToHost(InfoToSend);
        return;
    }

    // Get string data length
    int InfoLentgh = strlen(InfoToSend);
    if (InfoLentgh != 0)
    {
        // Add the current frame, then the data. Everything goes in ONE send()
        // call: the old code called send() twice, so 2 TCP packets were sent
        // every time.
        char frameText[20];
        snprintf(frameText, sizeof(frameText), "{%d;%d}", FRAME, frameCount);
        queueDataToServer(frameText, strlen(frameText));
        queueDataToServer(InfoToSend, InfoLentgh);
    }

    flushDataToServer();

    // UDP keep alive: until the server confirms, and then every second (the
    // server and the routers between us forget a silent UDP client)
    if (udpSocket >= 0 && udpToken != 0)
    {
        udpFramesSinceSend++;
        if (udpFramesSinceSend >= (udpConfirmed ? 60 : 30))
            udpSendDatagram("");
    }
}

/**
 * @brief Parse a string to an int
 *
 * @param charToParse
 * @return int
 */
int intParse(char *charToParse)
{
    int result = 0;
    sscanf(charToParse, "%d", &result);
    return result;
}

/**
 * @brief Parse a string to a float
 *
 * @param charToParse
 * @return int
 */
float floatParse(char *charToParse)
{
    float result = 0;
    sscanf(charToParse, "%f", &result);
    return result;
}

/**
 * @brief Get player from id
 *
 * @param id
 * @return Player*
 */
Player *GetPlayer(int id)
{
    for (int i = 0; i < MaxPlayer; i++)
    {
        if (AllPlayers[i].Id == id)
            return &AllPlayers[i];
    }
    return NULL;
}
//////////////////////////////////////////////////////////////////////////////
// Local multiplayer (NiFi)

/**
 * @brief Treat server messages now (host: messages of the clients converted to
 * server messages, like POS or SHOOT)
 *
 */
void networkTreatText(const char *text)
{
    networkAppendReceived(text);
    treatData();
}

/**
 * @brief Add server messages received from the host DS (treated in the game loop)
 *
 */
void networkAppendReceived(const char *text)
{
    int length = strlen(text);
    int current = strlen(Values);
    if (current + length >= (int)sizeof(Values))
    {
        netLog("Receive buffer full, data dropped");
        return;
    }
    memcpy(Values + current, text, length + 1);
}

void networkResetReceived(void)
{
    Values[0] = '\0';
}

/**
 * @brief Game loop of a client of a local multiplayer game (the host DS is the
 * server)
 *
 */
void runLocalClientGame(void)
{
    prepareParty(1);
    resetNetworkVariables();
    firstConnection = true;
    bool controllerMenuShown = false;

    while (Connection == LOCAL_GAME && localState == LOCAL_PLAYING)
    {
        // Messages of the host
        treatData();

        // The host gives the team: no team selection
        if (!controllerMenuShown && localPlayer->Id != UNUSED && localPlayer->Team != SPECTATOR && currentMenu == SCORE_BOARD)
        {
            controllerMenuShown = true;
            SetNeedChangeScreen(true);
            initControllerMenu();
        }

        // The camera shows our player when it is alive (the TEAM message
        // moves the camera to another player, like a player who joins an
        // online game during a round)
        if (localPlayer->Id != UNUSED && !localPlayer->IsDead && localPlayer->Team != SPECTATOR && CurrentCameraPlayer != 0)
            SetCurrentCameraPlayer(0);

        if (localPlayer->Id != UNUSED)
            GameLoop();

        UpdateEngine();
        sendDataToServer();
    }

    // End of the game decided by the host: back to the room
    if (localState == LOCAL_LOBBY)
    {
        Connection = UNSELECTED;
        localKeepRoom = true;
        initMainMenu();
        localKeepRoom = false;
        partyFinished = false;
        firstConnection = true;
        OnPartyQuit();
        initLocalLobbyMenu();
        return;
    }

    // End of the game (left by the player, or the host left)
    bool showError = currentMenu == ONLINE_ERROR;
    char savedError[ERROR_TEXT_LENGTH];
    strncpy(savedError, errorText, ERROR_TEXT_LENGTH);

    Connection = UNSELECTED;
    initMainMenu();
    if (showError)
    {
        strncpy(errorText, savedError, ERROR_TEXT_LENGTH);
        initOnlineErrorMenu();
    }
    partyFinished = false;
    firstConnection = true;
    OnPartyQuit();
}
