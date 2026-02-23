#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <variant>
#include <vector>
#include <cstring>

class PACKET_TILE_CHANGE_REQUEST {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet, ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {
        /* @debug
            [23:02:2026 13:55:19] [INFO ] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET] [PACKET_TILE_CHANGE_REQUEST:3]
            NetID:1|CharacterState:16|PlantingTree:4|X:874.00|Y:738.00|PunchX:25|PunchY:23|
        */

        if (Method == "SERVER -> PROXY -> CLIENT" && pm.NetID() == Player.netID && 
            pm.PlantingTree() > 0 &&  
            pm.X() > 0.00f && pm.Y() > 0.00f && 
            pm.PunchX() > 0 && pm.PunchY() > 0
        ) Player.RemoveItem(pm.PlantingTree(), 1);

        return true;
    }
};