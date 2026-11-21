// SPDX-License-Identifier: MIT
//
// Copyright (c) 2026, Counter-Strike DS contributors
//
// Local multiplayer (NiFi): DS to DS without access point, with the local
// multiplayer mode of DSWifi (BlocksDS). One DS is the host, up to 3 other DS
// join it. No bots in this mode.
//
// The consoles exchange the same text messages as the online mode
// ({NAME;param;param}). This file gives a reliable channel between the host
// and each client (sequence numbers, acks and resends).

#ifndef LOCALPLAY_H_
#define LOCALPLAY_H_

#include <stdbool.h>

#define LOCAL_MAX_PLAYERS 6                 // Host + 5 clients
#define LOCAL_MAX_CLIENTS (LOCAL_MAX_PLAYERS - 1)
#define LOCAL_MAX_ROOMS 4                   // Rooms shown in the list
#define LOCAL_NAME_LENGTH 15                // Same as PLAYER_MAX_LENGTH

enum LocalState
{
    LOCAL_OFF = 0,        // Local mode not used
    LOCAL_SCANNING = 1,   // Client: looking for rooms
    LOCAL_CONNECTING = 2, // Client: connecting to a room
    LOCAL_LOBBY = 3,      // Host or client: in the room, waiting for the start
    LOCAL_PLAYING = 4,    // Host or client: game started
};

// Reasons shown when the client leaves a room
enum LocalError
{
    LOCAL_ERROR_NONE = 0,
    LOCAL_ERROR_CANT_CONNECT = 1,
    LOCAL_ERROR_HOST_LEFT = 2,
    LOCAL_ERROR_ROOM_FULL = 3,
    LOCAL_ERROR_VERSION = 4,
    LOCAL_ERROR_WIFI = 5,
    LOCAL_ERROR_NO_ANSWER = 6, // Connected to the host, but no data received
    LOCAL_ERROR_KICKED = 7,    // The host removed our player
};

// Counters to check the connection (shown in the room menu)
typedef struct
{
    int handlerCalls;   // Calls of the packet handler (all packet types)
    int receivedFrames; // Data frames received by the packet handler
    int goodPackets;    // Packets of the game
    int badPackets;     // Frames without our header
    int sentPackets;
    int sendErrors;
    int lastSendError;
    int framesSinceLastPacket;
} LocalStats;

extern LocalStats localStats;

typedef struct
{
    char hostName[LOCAL_NAME_LENGTH + 1];
    int playersCurrent;
    int playersMax;
    bool allowsConnections;
} LocalRoomInfo;

typedef struct
{
    bool used;
    char name[LOCAL_NAME_LENGTH + 1];
} LocalLobbyPlayer;

extern enum LocalState localState;
extern bool localIsHost;
extern bool localKeepRoom; // Don't leave the room when the main menu is opened (back to the room)
extern enum LocalError localLastError;
extern LocalRoomInfo localRooms[LOCAL_MAX_ROOMS];
extern int localRoomCount;
extern LocalLobbyPlayer localLobbyPlayers[LOCAL_MAX_PLAYERS];

bool startWifiLibrary(void);
bool connectToAccessPoint(void);

void localCreateRoom(void);
void localStartScan(void);
void localJoinRoom(int roomIndex);
void localLeave(void);
void localReturnToLobby(void);
void localGiveLoadingTime(void);
void localUpdate(void);

// Send a text message (the host sends to all the clients)
void localSendToAll(const char *text);
// Client: send a text message to the host
void localSendToHost(const char *text);

const char *localGetErrorText(void);

// Host: clients of the room
bool localPeerIsInGame(int peerIndex);
const char *localPeerName(int peerIndex);
void localSendToPeer(int peerIndex, const char *text);

#endif // LOCALPLAY_H_
