// SPDX-License-Identifier: MIT
//
// Copyright (c) 2026, Counter-Strike DS contributors
//
// Local multiplayer (NiFi) with the local multiplayer mode of DSWifi.
//
// Transport:
// - The host DS uses Wifi_MultiplayerHostMode() and a beacon with our game id,
//   the clients find it with Wifi_ScanMode() and connect with
//   Wifi_ConnectOpenAP().
// - CMD/REPLY packets carry the text messages (like the local multiplayer
//   example of BlocksDS): the host sends one CMD packet per frame with a part
//   for each client (Wifi_MultiplayerHostCmdTxFrame()), the clients prepare
//   their REPLY packet every frame (Wifi_MultiplayerClientReplyTxFrame()) and
//   the hardware sends it when the CMD packet is received. Data frames are
//   not used anymore: with a lot of them, the RX/TX buffers of DSWiFi were
//   broken on the clients (assertions in wifi_arm9.c and ipc.c).
// - The part of a client in the CMD packet has the end of its MAC address
//   (the clients don't know their association id).
// - Each part has a small header with a sequence number and an ack: one
//   packet with data at a time per client, sent again until it is acked
//   (packets can be lost). The text is a stream: a message can be cut between
//   two packets.
//
// The wifi packet handlers are called in an interrupt: they only copy the
// packets in a queue, localUpdate() (main loop) treats them.

#include "main.h"
#include "localplay.h"
#include "network.h"
#include "ui.h"
#include "localhost.h"

#include <dswifi9.h>
#include <string.h>

// Game id in the beacon (only our rooms are shown)
#define LOCAL_GAME_ID 0x43534453 // "CSDS"

#define LOCAL_PACKET_MAGIC 0xC5
#define LOCAL_PACKET_MAGIC2 0x53 // Second byte of the header ("flags")
// The packet handlers may give the frame with the WiFi headers before our
// data: our header is searched in the first bytes of the frame
#define LOCAL_RX_EXTRA_BYTES 64
#define LOCAL_MAX_PAYLOAD 480             // Text in one packet (max)
#define LOCAL_CMD_SIZE 512                // Max size of the CMD packets (host -> clients)
#define LOCAL_REPLY_SIZE 64               // Size of the REPLY packets (client -> host)
#define LOCAL_RX_MAX (LOCAL_CMD_SIZE + LOCAL_RX_EXTRA_BYTES)
#define LOCAL_OUT_BUFFER_SIZE 2048        // Text waiting to be sent, per client
#define LOCAL_IN_BUFFER_SIZE 1024         // Text received and not treated yet (incomplete message)
#define LOCAL_RX_QUEUE_LENGTH 12          // Received packets waiting for treatment
#define LOCAL_TIMEOUT 600                 // Frames without receiving anything before a disconnection (10 s)
#define LOCAL_LOADING_GRACE 1800          // More time when the game starts (loading of the map, 30 s)
#define LOCAL_CONNECT_TIMEOUT 600         // Frames to connect to a room (10 s)
#define LOCAL_SCAN_REFRESH 30             // Frames between two updates of the rooms list

typedef struct
{
    u8 magic;
    u8 flags;
    u16 seq; // Sequence number of the data (0: no data)
    u16 ack; // Last sequence number received
    u16 length; // Text length
    u32 tag;    // CMD packets: end of the MAC address of the client (0 in REPLY packets)
} __attribute__((packed)) LocalPacketHeader;

#define LOCAL_PACKET_SIZE (sizeof(LocalPacketHeader) + LOCAL_MAX_PAYLOAD)

typedef struct
{
    bool used;
    int aid; // Association id (host: id of the client, client: 0 for the host)

    // Send
    char outBuffer[LOCAL_OUT_BUFFER_SIZE];
    int outLength;
    u8 inflight[LOCAL_PACKET_SIZE]; // Packet sent and not acked yet
    int inflightLength;
    bool inflightWaiting;
    u16 nextSeq;
    u32 tag; // Host: end of the MAC address of the client

    // Receive
    u16 lastReceivedSeq;
    char inStream[LOCAL_IN_BUFFER_SIZE]; // Incomplete message
    int inLength;
    int silentFrames;

    // Lobby
    bool helloReceived;
    char name[LOCAL_NAME_LENGTH + 1];
} LocalPeer;

typedef struct
{
    int aid;
    int length;
    u8 data[LOCAL_RX_MAX + 2];
} LocalRxPacket;

enum LocalState localState = LOCAL_OFF;
bool localIsHost = false;
bool localKeepRoom = false;
LocalStats localStats;
enum LocalError localLastError = LOCAL_ERROR_NONE;
LocalRoomInfo localRooms[LOCAL_MAX_ROOMS];
int localRoomCount = 0;
LocalLobbyPlayer localLobbyPlayers[LOCAL_MAX_PLAYERS];

static bool wifiStarted = false;
static LocalPeer peers[LOCAL_MAX_CLIENTS];
static Wifi_AccessPoint roomAccessPoints[LOCAL_MAX_ROOMS];
static int connectTimer = 0;
static int scanRefreshTimer = 0;
static bool allowNewClients = false;
static u32 myTag = 0;        // Client: end of our MAC address
static u32 myTagSwapped = 0; // Same with the bytes of each u16 swapped
static int cmdSize = LOCAL_CMD_SIZE;

static LocalRxPacket rxQueue[LOCAL_RX_QUEUE_LENGTH];
static volatile int rxQueueHead = 0; // Written by the interrupt
static volatile int rxQueueTail = 0; // Written by the main loop

static void treatLocalMessage(LocalPeer *peer, const char *message);
static void showLocalError(enum LocalError error);
static void sendLobbyToAll(void);

//////////////////////////////////////////////////////////////////////////////
// Wifi library

/**
 * @brief Start the wifi library once (local and online modes)
 *
 */
bool startWifiLibrary(void)
{
    if (!wifiStarted)
        wifiStarted = Wifi_InitDefault(INIT_ONLY);
    return wifiStarted;
}

/**
 * @brief Connect to the access point saved in the DS settings (online mode)
 *
 * @return true if connected
 */
bool connectToAccessPoint(void)
{
    if (!startWifiLibrary())
        return false;

    // Leave the local mode if it was used
    if (localState != LOCAL_OFF)
        localLeave();

    if (Wifi_AssocStatus() == ASSOCSTATUS_ASSOCIATED)
        return true;

    Wifi_InternetMode();
    for (int i = 0; i < 60 && !Wifi_LibraryModeReady(); i++)
        cothread_yield_irq(IRQ_VBLANK);

    Wifi_AutoConnect();
    // 30 seconds max (like the connection of Wifi_InitDefault(WFC_CONNECT))
    for (int i = 0; i < 60 * 30; i++)
    {
        int status = Wifi_AssocStatus();
        if (status == ASSOCSTATUS_ASSOCIATED)
            return true;
        if (status == ASSOCSTATUS_CANNOTCONNECT)
            return false;
        cothread_yield_irq(IRQ_VBLANK);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// Interrupt handlers

static void pushRxPacket(int aid, int base, int length)
{
    localStats.receivedFrames++;
    int next = (rxQueueHead + 1) % LOCAL_RX_QUEUE_LENGTH;
    if (next == rxQueueTail)
        return; // Queue full: the packet is lost, it will be sent again

    if (length < (int)sizeof(LocalPacketHeader))
        return;
    if (length > LOCAL_RX_MAX)
        length = LOCAL_RX_MAX;

    LocalRxPacket *packet = &rxQueue[rxQueueHead];
    packet->aid = aid;
    packet->length = length;
    // Read an even number of bytes (the buffer has 2 more bytes)
    Wifi_RxRawReadPacket(base, (length + 1) & ~1, packet->data);
    rxQueueHead = next;
}

static void fromClientHandler(Wifi_MPPacketType type, int aid, int base, int length)
{
    localStats.handlerCalls++;
    if (type == WIFI_MPTYPE_REPLY)
        pushRxPacket(aid, base, length);
}

static void fromHostHandler(Wifi_MPPacketType type, int base, int length)
{
    localStats.handlerCalls++;
    if (type == WIFI_MPTYPE_CMD)
        pushRxPacket(0, base, length);
}

//////////////////////////////////////////////////////////////////////////////
// Peers

static void resetPeer(LocalPeer *peer, int aid)
{
    memset(peer, 0, sizeof(LocalPeer));
    peer->used = true;
    peer->aid = aid;
    peer->nextSeq = 1;
}

static LocalPeer *getPeerByAid(int aid)
{
    for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
    {
        if (peers[i].used && peers[i].aid == aid)
            return &peers[i];
    }
    return NULL;
}

static void resetAll(void)
{
    for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
        peers[i].used = false;
    for (int i = 0; i < LOCAL_MAX_PLAYERS; i++)
        localLobbyPlayers[i].used = false;
    rxQueueTail = rxQueueHead;
    localRoomCount = 0;
    memset(&localStats, 0, sizeof(localStats));
}

/**
 * @brief Add a text message for a peer (sent in the next packets)
 *
 */
static void addToPeer(LocalPeer *peer, const char *text)
{
    if (!peer->used)
        return;

    // Add the complete messages one by one ({...}{...})
    const char *start = text;
    while ((start = strchr(start, '{')) != NULL)
    {
        const char *end = strchr(start, '}');
        if (end == NULL)
            break;

        int length = end - start + 1;
        if (peer->outLength + length < LOCAL_OUT_BUFFER_SIZE)
        {
            memcpy(peer->outBuffer + peer->outLength, start, length);
            peer->outLength += length;
            peer->outBuffer[peer->outLength] = '\0';
        }
        // else: too much data waiting (the peer doesn't answer), message lost
        start = end + 1;
    }
}

/**
 * @brief Write the part of a peer (header + data) in a packet
 *
 * The packet waiting for an ack is sent again every frame (the ack comes
 * back 2 frames later). If it doesn't fit, only the header (ack) is written.
 *
 * @param destination Where to write the part
 * @param maxBytes Space available
 * @param payloadLimit Max text in a new packet
 * @return Bytes written (multiple of 4), 0 if there is no space
 */
static int writePeerPart(LocalPeer *peer, u8 *destination, int maxBytes, int payloadLimit)
{
    if (payloadLimit > LOCAL_MAX_PAYLOAD)
        payloadLimit = LOCAL_MAX_PAYLOAD;

    // New packet with the waiting text (a message can be cut, the receiver
    // rebuilds the stream)
    if (!peer->inflightWaiting && peer->outLength > 0 && payloadLimit > 0)
    {
        int length = peer->outLength < payloadLimit ? peer->outLength : payloadLimit;
        LocalPacketHeader *header = (LocalPacketHeader *)peer->inflight;
        header->magic = LOCAL_PACKET_MAGIC;
        header->flags = LOCAL_PACKET_MAGIC2;
        header->seq = peer->nextSeq++;
        if (peer->nextSeq == 0)
            peer->nextSeq = 1;
        header->length = length;
        header->tag = 0;
        memcpy(peer->inflight + sizeof(LocalPacketHeader), peer->outBuffer, length);
        peer->inflightLength = sizeof(LocalPacketHeader) + length;
        peer->inflightWaiting = true;

        memmove(peer->outBuffer, peer->outBuffer + length, peer->outLength - length);
        peer->outLength -= length;
        peer->outBuffer[peer->outLength] = '\0';
    }

    int written;
    if (peer->inflightWaiting && ((peer->inflightLength + 3) & ~3) <= maxBytes)
    {
        written = peer->inflightLength;
        memcpy(destination, peer->inflight, written);
    }
    else if ((int)sizeof(LocalPacketHeader) <= maxBytes)
    {
        // Only the ack
        LocalPacketHeader header;
        header.magic = LOCAL_PACKET_MAGIC;
        header.flags = LOCAL_PACKET_MAGIC2;
        header.seq = 0;
        header.length = 0;
        header.tag = 0;
        written = sizeof(LocalPacketHeader);
        memcpy(destination, &header, written);
    }
    else
    {
        return 0;
    }

    LocalPacketHeader *header = (LocalPacketHeader *)destination;
    header->ack = peer->lastReceivedSeq;
    header->tag = localIsHost ? peer->tag : 0;

    // Size multiple of 4 (zeros at the end)
    int padded = (written + 3) & ~3;
    memset(destination + written, 0, padded - written);
    return padded;
}

/**
 * @brief Send the packet of this frame: one CMD packet (host) or the REPLY
 * packet (client)
 *
 */
static void sendFramePacket(void)
{
    if (localIsHost)
    {
        static u32 cmdBuffer[LOCAL_CMD_SIZE / 4];
        u8 *buffer = (u8 *)cmdBuffer;

        int count = 0;
        for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
        {
            if (peers[i].used)
                count++;
        }
        if (count == 0)
            return;

        // Same space for each client. The first client changes every frame
        // (if the packet of a client doesn't fit, it fits in the next frame)
        int share = cmdSize / count;
        int payloadLimit = share - (int)sizeof(LocalPacketHeader) - 4;
        static int first = 0;
        first = (first + 1) % LOCAL_MAX_CLIENTS;

        int total = 0;
        for (int j = 0; j < LOCAL_MAX_CLIENTS; j++)
        {
            LocalPeer *peer = &peers[(first + j) % LOCAL_MAX_CLIENTS];
            if (!peer->used)
                continue;
            total += writePeerPart(peer, buffer + total, cmdSize - total, payloadLimit);
        }

        if (total > 0)
        {
            int result = Wifi_MultiplayerHostCmdTxFrame(buffer, total);
            if (result < 0)
            {
                localStats.sendErrors++;
                localStats.lastSendError = result;
            }
            else
            {
                localStats.sentPackets++;
            }
        }
    }
    else
    {
        if (!peers[0].used || Wifi_AssocStatus() != ASSOCSTATUS_ASSOCIATED)
            return;

        static u32 replyBuffer[LOCAL_REPLY_SIZE / 4];
        u8 *buffer = (u8 *)replyBuffer;
        int total = writePeerPart(&peers[0], buffer, LOCAL_REPLY_SIZE, LOCAL_REPLY_SIZE - (int)sizeof(LocalPacketHeader));
        if (total > 0)
        {
            // Sent by the hardware when the next CMD packet of the host is
            // received
            int result = Wifi_MultiplayerClientReplyTxFrame(buffer, total);
            if (result < 0)
            {
                localStats.sendErrors++;
                localStats.lastSendError = result;
            }
            else
            {
                localStats.sentPackets++;
            }
        }
    }
}

/**
 * @brief Treat the received text of a peer (stream: messages can be cut)
 *
 */
static void treatPeerText(LocalPeer *peer, const u8 *payload, int length)
{
    if (peer->inLength + length >= LOCAL_IN_BUFFER_SIZE)
    {
        // Should not happen (messages are short): drop the broken data
        localStats.badPackets++;
        peer->inLength = 0;
        if (length >= LOCAL_IN_BUFFER_SIZE)
            return;
    }
    memcpy(peer->inStream + peer->inLength, payload, length);
    peer->inLength += length;
    peer->inStream[peer->inLength] = '\0';

    // Treat the complete messages
    int consumed = 0;
    while (true)
    {
        char *start = strchr(peer->inStream + consumed, '{');
        if (start == NULL)
        {
            consumed = peer->inLength;
            break;
        }
        char *end = strchr(start, '}');
        if (end == NULL)
        {
            consumed = start - peer->inStream;
            break;
        }
        *end = '\0';
        treatLocalMessage(peer, start + 1);
        // The peer can be removed by the message (error, leave)
        if (!peer->used)
            return;
        consumed = end + 1 - peer->inStream;
    }

    // Keep the incomplete message
    int left = peer->inLength - consumed;
    if (left > 0)
        memmove(peer->inStream, peer->inStream + consumed, left);
    peer->inLength = left;
    peer->inStream[peer->inLength] = '\0';
}

/**
 * @brief Treat one part (header + text) received from a peer
 *
 */
static void treatPeerPart(LocalPeer *peer, const LocalPacketHeader *header, const u8 *payload)
{
    localStats.goodPackets++;
    localStats.framesSinceLastPacket = 0;
    peer->silentFrames = 0;

    // Our packet was received
    LocalPacketHeader *inflightHeader = (LocalPacketHeader *)peer->inflight;
    if (peer->inflightWaiting && header->ack == inflightHeader->seq)
        peer->inflightWaiting = false;

    if (header->seq == 0 || header->length == 0)
        return;

    u16 expected = peer->lastReceivedSeq + 1;
    if (expected == 0)
        expected = 1;

    // Other sequence numbers: packet received again (our ack was lost), the
    // ack of the next packets tells it to the peer
    if (header->seq == expected)
    {
        peer->lastReceivedSeq = header->seq;
        treatPeerText(peer, payload, header->length);
    }
}

/**
 * @brief Treat a received packet (host: REPLY of a client, client: CMD of the
 * host with the parts of all the clients)
 *
 */
static void treatRxPacket(LocalRxPacket *packet)
{
    LocalPeer *peer = getPeerByAid(packet->aid);
    if (peer == NULL)
        return;

    // Find the first header (it can be after the WiFi headers)
    int offset = -1;
    for (int i = 0; i + (int)sizeof(LocalPacketHeader) <= packet->length && i <= LOCAL_RX_EXTRA_BYTES; i++)
    {
        if (packet->data[i] == LOCAL_PACKET_MAGIC && packet->data[i + 1] == LOCAL_PACKET_MAGIC2)
        {
            offset = i;
            break;
        }
    }
    if (offset < 0)
    {
        localStats.badPackets++;
        return;
    }

    // Each part (the REPLY packets have one part)
    while (offset + (int)sizeof(LocalPacketHeader) <= packet->length)
    {
        LocalPacketHeader header;
        memcpy(&header, packet->data + offset, sizeof(LocalPacketHeader));
        if (header.magic != LOCAL_PACKET_MAGIC || header.flags != LOCAL_PACKET_MAGIC2 || header.length > LOCAL_MAX_PAYLOAD ||
            offset + (int)sizeof(LocalPacketHeader) + header.length > packet->length)
        {
            if (offset == 0 || header.magic == LOCAL_PACKET_MAGIC)
                localStats.badPackets++;
            break;
        }

        const u8 *payload = packet->data + offset + sizeof(LocalPacketHeader);
        if (localIsHost || header.tag == myTag || header.tag == myTagSwapped)
        {
            treatPeerPart(peer, &header, payload);
            if (!localIsHost)
                break; // Our part was found
        }
        if (localIsHost)
            break;

        offset += (sizeof(LocalPacketHeader) + header.length + 3) & ~3;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Lobby messages
//
// Client -> host: {HELLO;version;name}
// Host -> clients: {LOBBY;name;name;...} (host first)
//                  {LERROR;error} (the client is kicked after that)

static void copyName(char *destination, const char *source)
{
    int i = 0;
    for (; source[i] != '\0' && i < LOCAL_NAME_LENGTH; i++)
    {
        char c = source[i];
        // No message separators in a name
        destination[i] = (c == ';' || c == '{' || c == '}') ? '_' : c;
    }
    destination[i] = '\0';
}

static void treatLocalMessage(LocalPeer *peer, const char *message)
{
    // Client in game: the messages of the host are server messages, treated
    // by network.c (except LSTART, start of the game)
    if (!localIsHost && localState == LOCAL_PLAYING)
    {
        // End of the game: back to the room
        if (strcmp(message, "LEND") == 0)
        {
            localState = LOCAL_LOBBY;
            return;
        }

        // The host removed our player (connection problem)
        if (strncmp(message, "LEAVE;", 6) == 0 && localPlayer->Id != UNUSED && atoi(message + 6) == localPlayer->Id)
        {
            showLocalError(LOCAL_ERROR_KICKED);
            return;
        }

        char text[LOCAL_MAX_PAYLOAD + 3];
        snprintf(text, sizeof(text), "{%s}", message);
        networkAppendReceived(text);
        return;
    }

    // Split the parameters
    char copy[LOCAL_MAX_PAYLOAD + 1];
    strncpy(copy, message, LOCAL_MAX_PAYLOAD);
    copy[LOCAL_MAX_PAYLOAD] = '\0';

    // HIT has 4 values per hit (up to 8 hits with a shotgun)
    char *params[40];
    int paramCount = 0;
    char *saveptr = NULL;
    char *token = strtok_r(copy, ";", &saveptr);
    while (token != NULL && paramCount < 40)
    {
        params[paramCount++] = token;
        token = strtok_r(NULL, ";", &saveptr);
    }
    if (paramCount == 0)
        return;

    if (localIsHost)
    {
        if (strcmp(params[0], "HELLO") != 0)
        {
            // Game message of a client (online client format)
            localHostTreatClientMessage(peer - peers, params, paramCount);
        }
        else if (paramCount >= 3)
        {
            if (strcmp(params[1], GAME_VERSION) != 0)
            {
                char text[24];
                snprintf(text, sizeof(text), "{LERROR;%d}", LOCAL_ERROR_VERSION);
                addToPeer(peer, text);
                return;
            }
            peer->helloReceived = true;
            copyName(peer->name, params[2]);
            sendLobbyToAll();
        }
    }
    else
    {
        if (strcmp(params[0], "LOBBY") == 0)
        {
            for (int i = 0; i < LOCAL_MAX_PLAYERS; i++)
            {
                localLobbyPlayers[i].used = i + 1 < paramCount;
                if (localLobbyPlayers[i].used)
                    copyName(localLobbyPlayers[i].name, params[i + 1]);
            }
            if (localState == LOCAL_CONNECTING)
                localState = LOCAL_LOBBY;
        }
        else if (strcmp(params[0], "LSTART") == 0)
        {
            // The host starts the game: the next messages are server messages
            localState = LOCAL_PLAYING;
            localGiveLoadingTime();
            networkResetReceived();
            Connection = LOCAL_GAME;
        }
        else if (strcmp(params[0], "LERROR") == 0 && paramCount >= 2)
        {
            showLocalError((enum LocalError)atoi(params[1]));
        }
    }
}

/**
 * @brief Host: update the lobby players and send them to all the clients
 *
 */
static void sendLobbyToAll(void)
{
    localLobbyPlayers[0].used = true;
    copyName(localLobbyPlayers[0].name, localPlayer->name);

    char text[LOCAL_MAX_PAYLOAD];
    snprintf(text, sizeof(text), "{LOBBY;%s", localLobbyPlayers[0].name);

    int index = 1;
    for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
    {
        if (peers[i].used && peers[i].helloReceived)
        {
            localLobbyPlayers[index].used = true;
            strncpy(localLobbyPlayers[index].name, peers[i].name, LOCAL_NAME_LENGTH + 1);
            snprintf(text + strlen(text), sizeof(text) - strlen(text), ";%s", peers[i].name);
            index++;
        }
    }
    for (; index < LOCAL_MAX_PLAYERS; index++)
        localLobbyPlayers[index].used = false;

    snprintf(text + strlen(text), sizeof(text) - strlen(text), "}");
    localSendToAll(text);
}

//////////////////////////////////////////////////////////////////////////////
// Public functions

/**
 * @brief Create a room (this DS is the host)
 *
 */
void localCreateRoom(void)
{
    localLeave();
    localLastError = LOCAL_ERROR_NONE;

    if (!startWifiLibrary())
    {
        localLastError = LOCAL_ERROR_WIFI;
        return;
    }

    resetAll();
    localIsHost = true;

    // Same order as the local multiplayer example of BlocksDS (CMD/REPLY
    // packets)
    // packets). The biggest CMD size accepted by DSWiFi is used, the errors
    // are shown in the counters (e:count(last error))
    static const int cmdSizes[] = {LOCAL_CMD_SIZE, 384, 256, 128};
    for (int i = 0; i < 4; i++)
    {
        cmdSize = cmdSizes[i];
        int result = Wifi_MultiplayerHostMode(LOCAL_MAX_CLIENTS, cmdSize, LOCAL_REPLY_SIZE);
        if (result >= 0)
            break;
        localStats.sendErrors++;
        localStats.lastSendError = result;
    }
    Wifi_MultiplayerFromClientSetPacketHandler(fromClientHandler);
    for (int i = 0; i < 120 && !Wifi_LibraryModeReady(); i++)
        cothread_yield_irq(IRQ_VBLANK);

    Wifi_SetChannel(6);
    allowNewClients = true;
    Wifi_MultiplayerAllowNewClients(true);
    Wifi_BeaconStart("CSDS", LOCAL_GAME_ID);
    cothread_yield_irq(IRQ_VBLANK);
    cothread_yield_irq(IRQ_VBLANK);

    localState = LOCAL_LOBBY;
    sendLobbyToAll();
}

/**
 * @brief Look for rooms (this DS is a client)
 *
 */
void localStartScan(void)
{
    localLeave();
    localLastError = LOCAL_ERROR_NONE;

    if (!startWifiLibrary())
    {
        localLastError = LOCAL_ERROR_WIFI;
        return;
    }

    resetAll();
    localIsHost = false;

    int result = Wifi_MultiplayerClientMode(LOCAL_REPLY_SIZE);
    if (result < 0)
    {
        localStats.sendErrors++;
        localStats.lastSendError = result;
    }

    // Our part in the CMD packets of the host: end of our MAC address
    u8 mac[6] = {0};
    Wifi_GetData(WIFIGETDATA_MACADDRESS, 6, mac);
    myTag = mac[2] | (mac[3] << 8) | (mac[4] << 16) | ((u32)mac[5] << 24);
    myTagSwapped = mac[3] | (mac[2] << 8) | (mac[5] << 16) | ((u32)mac[4] << 24);
    for (int i = 0; i < 120 && !Wifi_LibraryModeReady(); i++)
        cothread_yield_irq(IRQ_VBLANK);

    Wifi_MultiplayerFromHostSetPacketHandler(fromHostHandler);
    Wifi_ScanMode();

    scanRefreshTimer = 0;
    localState = LOCAL_SCANNING;
}

/**
 * @brief Update the list of the rooms found by the scan
 *
 */
static void updateRoomsList(void)
{
    localRoomCount = 0;
    int count = Wifi_GetNumAP();
    for (int i = 0; i < count && localRoomCount < LOCAL_MAX_ROOMS; i++)
    {
        Wifi_AccessPoint ap;
        if (Wifi_GetAPData(i, &ap) != 0)
            continue;

        if (!(ap.flags & WFLAG_APDATA_NINTENDO_TAG) || ap.nintendo.game_id != LOCAL_GAME_ID)
            continue;

        LocalRoomInfo *room = &localRooms[localRoomCount];
        roomAccessPoints[localRoomCount] = ap;

        // Name in UTF-16: keep the ASCII characters
        int length = ap.nintendo.name_len;
        if (length > LOCAL_NAME_LENGTH)
            length = LOCAL_NAME_LENGTH;
        if (length > 10)
            length = 10;
        int j = 0;
        for (int k = 0; k < length; k++)
        {
            u16 c = ap.nintendo.name[k];
            room->hostName[j++] = (c >= 32 && c < 127) ? (char)c : '?';
        }
        room->hostName[j] = '\0';

        room->playersCurrent = ap.nintendo.players_current;
        room->playersMax = ap.nintendo.players_max;
        room->allowsConnections = ap.nintendo.allows_connections;
        localRoomCount++;
    }
}

/**
 * @brief Join a room of the list
 *
 */
void localJoinRoom(int roomIndex)
{
    if (localState != LOCAL_SCANNING || roomIndex < 0 || roomIndex >= localRoomCount)
        return;

    if (!localRooms[roomIndex].allowsConnections)
    {
        showLocalError(LOCAL_ERROR_ROOM_FULL);
        return;
    }

    resetPeer(&peers[0], 0);
    Wifi_ConnectOpenAP(&roomAccessPoints[roomIndex]);
    connectTimer = LOCAL_CONNECT_TIMEOUT;
    localState = LOCAL_CONNECTING;
}

/**
 * @brief Leave the local mode (room, scan or game)
 *
 */
void localLeave(void)
{
    if (localState == LOCAL_OFF)
        return;

    // The clients are told that the host leaves, the host is told that the
    // client leaves
    Wifi_IdleMode();
    resetAll();
    localState = LOCAL_OFF;
    localIsHost = false;
}

/**
 * @brief Host: end of the game, everybody goes back to the room
 *
 */
void localReturnToLobby(void)
{
    if (!localIsHost || localState != LOCAL_PLAYING)
        return;

    localState = LOCAL_LOBBY;
    localSendToAll("{LEND}");
    sendLobbyToAll();
}

/**
 * @brief The game starts: the map is loaded, nothing is received or sent
 * during the loading. Don't disconnect the other consoles because of that.
 *
 */
void localGiveLoadingTime(void)
{
    for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
        peers[i].silentFrames = -LOCAL_LOADING_GRACE;
}

void localSendToAll(const char *text)
{
    for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
    {
        if (peers[i].used && (peers[i].helloReceived || strncmp(text, "{LERROR", 7) == 0))
            addToPeer(&peers[i], text);
    }
}

void localSendToHost(const char *text)
{
    if (!localIsHost)
        addToPeer(&peers[0], text);
}

bool localPeerIsInGame(int peerIndex)
{
    return localIsHost && peerIndex >= 0 && peerIndex < LOCAL_MAX_CLIENTS && peers[peerIndex].used && peers[peerIndex].helloReceived;
}

const char *localPeerName(int peerIndex)
{
    if (peerIndex < 0 || peerIndex >= LOCAL_MAX_CLIENTS)
        return "";
    return peers[peerIndex].name;
}

void localSendToPeer(int peerIndex, const char *text)
{
    if (localPeerIsInGame(peerIndex))
        addToPeer(&peers[peerIndex], text);
}

/**
 * @brief Host: add the new clients and remove the clients who left
 *
 */
static void updateHostClients(void)
{
    u16 mask = Wifi_MultiplayerGetClientMask();
    bool changed = false;

    // Clients who left
    for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
    {
        if (peers[i].used && (!(mask & (1 << peers[i].aid)) || peers[i].silentFrames > LOCAL_TIMEOUT))
        {
            if (mask & (1 << peers[i].aid))
                Wifi_MultiplayerKickClientByAID(peers[i].aid);
            peers[i].used = false;
            // Remove its player from the game
            localHostOnPeerLeft(i);
            changed = true;
        }
    }

    // New clients
    for (int aid = 1; aid < 16; aid++)
    {
        if (!(mask & (1 << aid)) || getPeerByAid(aid) != NULL)
            continue;

        int freeIndex = -1;
        for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
        {
            if (!peers[i].used)
            {
                freeIndex = i;
                break;
            }
        }

        // No game in progress for new players for now, and the room is full
        if (freeIndex == -1 || localState != LOCAL_LOBBY)
        {
            Wifi_MultiplayerKickClientByAID(aid);
            continue;
        }
        resetPeer(&peers[freeIndex], aid);

        // Its part in the CMD packets: end of its MAC address
        Wifi_ConnectedClient clients[15];
        int count = Wifi_MultiplayerGetClients(15, clients);
        for (int i = 0; i < count; i++)
        {
            if (clients[i].association_id == aid)
            {
                const u8 *mac = (const u8 *)clients[i].macaddr;
                peers[freeIndex].tag = mac[2] | (mac[3] << 8) | (mac[4] << 16) | ((u32)mac[5] << 24);
                break;
            }
        }
    }

    // Wifi_MultiplayerAllowNewClients(false) is not used on purpose: it
    // changes the beacon while the game is running, and the host crashed
    // inside the DSWiFi beacon code (Data abort). New clients are always
    // accepted by DSWiFi and kicked above if the room is full or the game
    // started.
    (void)allowNewClients;

    if (changed)
        sendLobbyToAll();
}

/**
 * @brief Local mode update (call it every frame, in the menus and in game)
 *
 */
void localUpdate(void)
{
    if (localState == LOCAL_OFF)
        return;

    // DSWiFi treats the received packets in its own thread (like lwIP for the
    // online mode, see ReadServerData()): it only runs when the main thread
    // yields. Without this, the packet handlers are never called.
    cothread_yield();

    // The bottom screen is only drawn when something changes: draw the room
    // menus regularly (players, rooms). Not during the game: each redraw
    // switches the 3D to both screens for a few frames, and the models of the
    // players blinked on the top screen (the counters of the gamepad menu are
    // updated with the other changes of the bottom screen).
    static int redrawTimer = 0;
    if (++redrawTimer >= 15)
    {
        redrawTimer = 0;
        if ((currentMenu == LOCAL_ROOMS || currentMenu == LOCAL_LOBBY_MENU) && UpdateBottomScreenFrameCount < 2)
            UpdateBottomScreenFrameCount = 2;
    }

    // Received packets
    while (rxQueueTail != rxQueueHead)
    {
        treatRxPacket(&rxQueue[rxQueueTail]);
        rxQueueTail = (rxQueueTail + 1) % LOCAL_RX_QUEUE_LENGTH;
    }

    if (localState == LOCAL_SCANNING)
    {
        if (scanRefreshTimer-- <= 0)
        {
            scanRefreshTimer = LOCAL_SCAN_REFRESH;
            updateRoomsList();
        }
        return;
    }

    if (localIsHost)
    {
        updateHostClients();
    }
    else
    {
        int status = Wifi_AssocStatus();
        if (localState == LOCAL_CONNECTING)
        {
            if (status == ASSOCSTATUS_ASSOCIATED && !peers[0].helloReceived)
            {
                // Connected: say hello to the host
                peers[0].helloReceived = true;
                char text[64];
                snprintf(text, sizeof(text), "{HELLO;%s;%s}", GAME_VERSION, localPlayer->name);
                addToPeer(&peers[0], text);
            }
            else if (status == ASSOCSTATUS_CANNOTCONNECT || --connectTimer <= 0)
            {
                // Connected but no answer of the host, or not connected
                showLocalError(peers[0].helloReceived ? LOCAL_ERROR_NO_ANSWER : LOCAL_ERROR_CANT_CONNECT);
                return;
            }
        }
        else if (status != ASSOCSTATUS_ASSOCIATED || peers[0].silentFrames > LOCAL_TIMEOUT)
        {
            // The host left
            showLocalError(LOCAL_ERROR_HOST_LEFT);
            return;
        }
    }

    localStats.framesSinceLastPacket++;

    for (int i = 0; i < LOCAL_MAX_CLIENTS; i++)
    {
        if (peers[i].used)
            peers[i].silentFrames++;
    }

    // Send the data (one packet per frame)
    sendFramePacket();

    // Let DSWiFi send the packets now
    cothread_yield();
}

/**
 * @brief Leave the local mode and show the error
 *
 */
static void showLocalError(enum LocalError error)
{
    // Counters of the connection (to find the problem), before localLeave()
    LocalStats stats = localStats;

    localLeave();
    localLastError = error;
    snprintf(errorText, ERROR_TEXT_LENGTH, "%s h:%d rx:%d/%d/%d tx:%d err:%d(%d)", localGetErrorText(), stats.handlerCalls,
             stats.receivedFrames, stats.goodPackets, stats.badPackets, stats.sentPackets, stats.sendErrors, stats.lastSendError);
    initOnlineErrorMenu();
}

/**
 * @brief Text of the last error (online error menu)
 *
 */
const char *localGetErrorText(void)
{
    switch (localLastError)
    {
    case LOCAL_ERROR_CANT_CONNECT:
        return "Can't join the room.";
    case LOCAL_ERROR_KICKED:
        return "You were disconnected from the room.";
    case LOCAL_ERROR_NO_ANSWER:
        return "Connected, but the host doesn't answer.";
    case LOCAL_ERROR_HOST_LEFT:
        return "The host left the room.";
    case LOCAL_ERROR_ROOM_FULL:
        return "The room is full or the game has started.";
    case LOCAL_ERROR_VERSION:
        return "The host has another version of the game.";
    case LOCAL_ERROR_WIFI:
        return "Can't start the wifi.";
    default:
        return "";
    }
}
