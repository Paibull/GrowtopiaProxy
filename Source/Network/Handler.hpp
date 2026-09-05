#pragma once

#include "ENET/enet.h"
#include <string>

int GetPacketType(const ENetPacket* packet);
std::string GetPacketText(ENetPacket* packet);
std::string GetPacketHex(const ENetPacket* packet);
void SendPacket(ENetPeer* peer, int num, const char* data, size_t len);

bool HandlePacket(int type, ENetPacket* packet, ENetPeer* to);

/* Runs a slash command as though the player had typed it, against whatever
   session is live. Returns false when there is none. Used by the local control
   endpoint so a command can be driven without the game window. */
bool RunProxyCommand(const std::string& command);