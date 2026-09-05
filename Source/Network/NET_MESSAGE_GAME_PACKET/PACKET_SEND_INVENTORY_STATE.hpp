#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <variant>
#include <vector>
#include <cstring>

class PACKET_SEND_INVENTORY_STATE {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet, ENetPeer* CLIENT, ENetPeer* SERVER) {
        if (packet->dataLength < 67) return true;

        int inv_size;
        std::memcpy(&inv_size, packet->data + 61, 4);

        const size_t maxEntries = (packet->dataLength - 67) / 4;
        if (inv_size < 0) inv_size = 0;
        if (static_cast<size_t>(inv_size) > maxEntries) inv_size = static_cast<int>(maxEntries);

        size_t offset = 67;

        std::vector<std::pair<int, int>> itemsVec;

        for (int i = 0; i < inv_size; i++) {
            if (offset + 4 > packet->dataLength) break;

            uint16_t itemID;
            uint8_t count;

            std::memcpy(&itemID, packet->data + offset, 2);
            std::memcpy(&count, packet->data + offset + 2, 1);

            offset += 4;

            itemsVec.emplace_back(itemID, count);
        }

        Player.ResetInventory(inv_size, itemsVec);

        return true;
    }
};