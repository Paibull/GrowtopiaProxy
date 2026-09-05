#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

struct TileExtra {
    uint8_t type;
};

struct Tile {
    uint16_t fg;
    uint16_t bg;
    uint16_t parent;
    uint16_t flag;
    uint16_t lock_parent;
    TileExtra extra;
};

inline void parse_tile(const uint8_t*& ptr, const uint8_t* buffer_end, int x, int y) {
    Tile tile{};


}

// World packet parse sınıfı
class PACKET_SEND_MAP_DATA {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet,
        ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {

        if (Type != 4 || !packet || !packet->data) return true;

        const uint8_t* data = packet->data;
        const size_t size = packet->dataLength;

        if (size < 78) return true;

        uint8_t name_len = data[66];
        if (size < 78u + name_len) return true;

        std::string world_name((const char*)(data + 68), name_len);

        uint8_t xSize = data[68 + name_len];
        uint8_t ySize = data[72 + name_len];
        uint16_t block_count = 0;
        std::memcpy(&block_count, data + 76 + name_len, sizeof(block_count));

        LOG_DEBUG("World Name: {}", world_name);
        LOG_DEBUG("World Size: {} x {}", xSize, ySize);
        LOG_DEBUG("Total Blocks: {}", block_count);

        Player.world = world_name;


        return true;
    }
};