// SPDX-License-Identifier: MIT
//
// Copyright (c) 2026, Counter-Strike DS contributors
//
// Local multiplayer (NiFi), host side: the host DS plays with the rules of the
// single player mode (without bots) and acts as the server for the clients.
// It sends the same messages as the online server, so the clients use the
// online code (network.c).

#ifndef LOCALHOST_H_
#define LOCALHOST_H_

#include <stdbool.h>

// Is this DS the host of a local game in progress?
bool isLocalHostGame(void);

void localHostAddPlayers(void);
void localHostStartGame(void);
void localHostUpdate(void);
void localHostEndGame(void);

// Called by localplay.c
void localHostTreatClientMessage(int peerIndex, char **params, int paramCount);
void localHostOnPeerLeft(int peerIndex);

// Called by droppedGuns.c: a player took a gun on the floor
void localHostOnGunPickup(int playerIndex);

#endif // LOCALHOST_H_
