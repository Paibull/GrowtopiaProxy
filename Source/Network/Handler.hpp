#pragma once

#include "ENET/enet.h"
#include <string>

int GetPacketType(const ENetPacket* packet);
std::string GetPacketText(ENetPacket* packet);
std::string GetPacketHex(const ENetPacket* packet);
void SendPacket(ENetPeer* peer, int num, const char* data, int len);

bool HandlePacket(int type, ENetPacket* packet, ENetPeer* to);