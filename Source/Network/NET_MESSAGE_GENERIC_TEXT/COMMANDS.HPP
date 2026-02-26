#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"
#include "../../Player/Player.hpp"

#include <variant>
#include <vector>
#include <cstring>

class COMMANDS {
public:
    static inline bool Inject(std::string command, ENetPeer* CLIENT, ENetPeer* SERVER) {
        
        if (Packet::Contains(command, "/ping")) {
            int serverPing = SERVER ? SERVER->roundTripTime : 0;

            int clientPing = 0;
            if (Local.IP == "127.0.0.1") clientPing = 0;
            else clientPing = CLIENT ? CLIENT->roundTripTime : 0;

            GamePacket<OnConsoleMessage> p;
            p.text = "Your ping is " + std::to_string(serverPing + clientPing) + " (C" + std::to_string(clientPing) + "+S" + std::to_string(serverPing) + ")";
            SendGamePacket(CLIENT, p);

            return false;
        }
        else if (Packet::Contains(command, "/inventory")) {
            std::string inv = Player.InventoryToString();

            LOG_WARN("Player Inventory:\n{}", inv);

            GamePacket<OnConsoleMessage> p;
            p.text = "`8Player Inventory:\n" + inv;
            SendGamePacket(CLIENT, p);

            return false;
        }
        else if (Packet::Contains(command, "/wd ") || Packet::Contains(command, "/dd ") || Packet::Contains(command, "/bd ")) {
            std::string item_name = "World Lock";
            int item_id = 242;

            if (Packet::Contains(command, "/dd ")) item_id = 1796, item_name = "Diamond Lock";
            else if (Packet::Contains(command, "/bd ")) item_id = 7188, item_name = "Blue Gem Lock";

            int item_count = Packet::ExtractCustom<int>(command, " ", 1, "\n");
            if (item_count < 1) return false;
            if (item_count > 200) item_count = 200;
            if (item_count > 1) item_name += "s";

            Player.Drop(item_id, item_count, item_name, CLIENT, SERVER);

            return false;
        }

        return true;
    }
};