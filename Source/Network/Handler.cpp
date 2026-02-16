#include "Handler.hpp"
#include "Network.hpp"
#include "../Main/Config.hpp"
#include "../Logger/Logger.hpp"
#include "../Functions/Functions.hpp"
#include "../Packets/Packets.hpp"

#include <iomanip>
#include <sstream>
#include <vector>
#include <variant>

int GetPacketType(const ENetPacket* packet) {
    if (!packet || packet->dataLength < 1) return -1;
    return *(int*)(packet->data);
}

std::string GetPacketText(ENetPacket* packet) {
    if (!packet) return "";
    char zero = 0;
    memcpy(packet->data + packet->dataLength - 1, &zero, 1);
    return std::string((char*)(packet->data + 4));
}

std::string GetPacketHex(const ENetPacket* packet) {
    if (!packet || packet->dataLength == 0) return "unknown packet";
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < packet->dataLength; ++i) oss << std::setw(2) << static_cast<unsigned>(packet->data[i]) << " ";
    return oss.str();
}

void SendPacket(ENetPeer* peer, int num, const char* data, int len) {
    ENetPacket* packet = enet_packet_create(nullptr, len + 5, ENET_PACKET_FLAG_RELIABLE);
    memcpy(packet->data, &num, 4);
    if (data && len > 0) memcpy(packet->data + 4, data, len);
    packet->data[4 + len] = 0x00;
    enet_peer_send(peer, 0, packet);
    /* SendPacket(event.peer, type, string.c_str(), string.length()); */
}

template<typename T>
T ReadPacketType(const uint8_t* data, size_t& cursor) {
    T value;
    std::memcpy(&value, data + cursor, sizeof(T));
    cursor += sizeof(T);
    return value;
}

bool HandlePacket(int type, ENetPacket* packet, ENetPeer* to) {
    /* @important: if you want to forward the packet without editing (the original) then use 'return true;'
        if you want to edit, after editing the packet, use 'return false;' so it won't send the original packet but your custom packet
    */

    if (!packet) return true;

    std::string Method;
    if (type == Network.CLIENT_PROXY_SERVER) Method = "CLIENT -> PROXY -> SERVER";
    else if (type == Network.SERVER_PROXY_CLIENT) Method = "SERVER -> PROXY -> CLIENT";

    int Type = GetPacketType(packet);

    switch (Type) {
        case Network.NET_MESSAGE_SERVER_HELLO: {
            /* @info: Don't edit this packet */
            if (ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), GetPacketHex(packet));
            return true;
        }

        case Network.NET_MESSAGE_GENERIC_TEXT: {
            if (ShouldLogNetMessage(Type)) {
                std::string text = GetPacketText(packet);
                LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);
            }

            /* @info: Edit here */

            return true;
        }

        case Network.NET_MESSAGE_GAME_MESSAGE: {
            if (ShouldLogNetMessage(Type)) {
                std::string text = GetPacketText(packet);
                LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);
            }

            /* @info: Edit here */

            return true;
        }

        case Network.NET_MESSAGE_GAME_PACKET: {
            std::string debug, text;

            size_t cursor = 61;
            int variantCount = packet->data[60];
            if (variantCount < 0 || variantCount > 100) variantCount = 100;
            int readCount = 0;

            struct Vec2 { float x; float y; };
            struct Vec3 { float x; float y; float z; };

            using Variant = std::variant<std::string, int, unsigned int, float, long long, Vec2, Vec3>;
            std::vector<Variant> param;

            if (packet->dataLength > 61) {
                while (cursor + 2 <= packet->dataLength && readCount < variantCount) {
                    BYTE idx = packet->data[cursor];
                    BYTE vtype = packet->data[cursor + 1];
                    cursor += 2;

                    readCount++;

                    switch (vtype) {
                        case 0x2: { // STRING
                            if (cursor + sizeof(int) > packet->dataLength) return true;
                            int slen = ReadPacketType<int>(packet->data, cursor);
                            if (slen < 0 || cursor + slen > packet->dataLength) return true;

                            std::string value((char*)packet->data + cursor, slen);
                            cursor += slen;

                            debug += "[" + std::to_string(idx) + "][STRING] " + value + "\n";
                            text += value + "\n";

                            if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_PLAYER_MOVING_CLEAR_SPAM) && idx == 0 && value == "OnClearItemTransforms") return true;

                            param.push_back(value);
                            break;
                        }
                        case 0x1: { // FLOAT
                            if (cursor + sizeof(float) > packet->dataLength) return true;
                            float v = ReadPacketType<float>(packet->data, cursor);

                            debug += "[" + std::to_string(idx) + "][FLOAT] " + std::to_string(v) + "\n";
                            text += std::to_string(v) + "\n";

                            param.push_back(v);
                            break;
                        }
                        case 0x3: { // Vec2
                            if (cursor + sizeof(float) * 2 > packet->dataLength) return true;
                            Vec2 custom;
                            custom.x = ReadPacketType<float>(packet->data, cursor);
                            custom.y = ReadPacketType<float>(packet->data, cursor);

                            debug += "[" + std::to_string(idx) + "][FLOAT2] " + std::to_string(custom.x) + ", " + std::to_string(custom.y) + "\n";
                            text += std::to_string(custom.x) + "," + std::to_string(custom.y) + "\n";

                            param.push_back(custom);
                            break;
                        }
                        case 0x4: { // Vec3
                            if (cursor + sizeof(float) * 3 > packet->dataLength) return true;
                            Vec3 custom;
                            custom.x = ReadPacketType<float>(packet->data, cursor);
                            custom.y = ReadPacketType<float>(packet->data, cursor);
                            custom.z = ReadPacketType<float>(packet->data, cursor);

                            debug += "[" + std::to_string(idx) + "][FLOAT3] " + std::to_string(custom.x) + ", " + std::to_string(custom.y) + ", " + std::to_string(custom.z) + "\n";
                            text += std::to_string(custom.x) + "," + std::to_string(custom.y) + "," + std::to_string(custom.z) + "\n";

                            param.push_back(custom);
                            break;
                        }
                        case 0x5: { // UINT
                            if (cursor + sizeof(unsigned int) > packet->dataLength) return true;
                            unsigned int v = ReadPacketType<unsigned int>(packet->data, cursor);

                            debug += "[" + std::to_string(idx) + "][UINT] " + std::to_string(v) + "\n";
                            text += std::to_string(v) + "\n";

                            param.push_back(v);
                            break;
                        }
                        case 0x6: { // INT64
                            if (cursor + sizeof(long long) > packet->dataLength) return true;
                            long long v = ReadPacketType<long long>(packet->data, cursor);

                            debug += "[" + std::to_string(idx) + "][INT64] " + std::to_string(v) + "\n";
                            text += std::to_string(v) + "\n";

                            param.push_back(v);
                            break;
                        }
                        case 0x9: { // INT
                            if (cursor + sizeof(int) > packet->dataLength) return true;
                            int v = ReadPacketType<int>(packet->data, cursor);

                            debug += "[" + std::to_string(idx) + "][INT] " + std::to_string(v) + "\n";
                            text += std::to_string(v) + "\n";

                            param.push_back(v);
                            break;
                        }
                        default: {
                            /* @important: sometimes there are empty bytes are sent for example '0x0' & '0x40'.

                                You can Log them with this line: 
                                    LOG_ERROR("Unknown variant type: 0x{:X}\nAll Buffer: {}", vtype, GetPacketHex(packet));

                                Also you need to delate 'OnClearItemTransforms' from the above (OnClearItemTransforms is just one of many).
                                    To find "ctrl + f" and write 'Settings.SkipTrashPacketsDebug'
                            */ 
                            return true;
                        }
                    }
                }
            }

            if (!debug.empty()) {
                if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), debug);

                /* @info: Edit here  */
                if (!param.empty()) {
                    if (auto pStr = std::get_if<std::string>(&param[0])) {
                        if (std::get<std::string>(param[0]) == "OnDialogRequest" && param.size() == 2) {
                            GamePacket<OnDialogRequest> p;
                            p.text = std::get<std::string>(param[1]);
                            SendGamePacket(to, p);
                            return false;
                        }
                        else if (std::get<std::string>(param[0]) == "OnConsoleMessage" && param.size() == 2) {
                            GamePacket<OnConsoleMessage> p;
                            p.text = "`o[`4GrowtopiaProxy`o]`w`w`w`w`w`w`w`w`w`w " + std::get<std::string>(param[1]);
                            SendGamePacket(to, p);
                            return false;
                        }
                        else if (std::get<std::string>(param[0]) == "OnSendToServer" && param.size() >= 6) {
                            /* @debug:
                                [0][STRING] OnSendToServer
                                [1][INT] 17040
                                [2][INT] 4744778
                                [3][INT] 184573909
                                [4][STRING] 213.179.209.175|0|02560A89C0D794A90985ADA5C18D8AA6
                                [5][INT] 1
                                [6][STRING] GrowtopiaProxy
                            */

                            {
                                ENetPeer* old_server_peer = Network.client_to_server[to];
                                if (old_server_peer) enet_peer_disconnect_now(old_server_peer, 0);
                            }

                            {
                                uint16_t newServerUDP = static_cast<uint16_t>(std::get<int>(param[1]));
                                std::string newServerIP = Packet::ExtractCustom<std::string>(std::get<std::string>(param[4]), "", 0, "|");

                                enet_address_set_host(&Network.GetServerAddress(), newServerIP.c_str());
                                Network.GetServerAddress().port = newServerUDP;
                            }

                            {
                                GamePacket<OnSendToServer> p;
                                if (param.size() >= 2) p.port = Local.UDP;
                                if (param.size() >= 3) p.token = std::get<int>(param[2]);
                                if (param.size() >= 4) p.userId = std::get<int>(param[3]);
                                if (param.size() >= 5) p.ip = Local.IP;
                                if (param.size() >= 5) p.doorID = Packet::ExtractCustom<std::string>(std::get<std::string>(param[4]), "|", 1, "|");
                                if (param.size() >= 5) p.UUIDToken = Packet::ExtractCustom<std::string>(std::get<std::string>(param[4]), "|", 2, "|");
                                if (param.size() >= 6) p.lmode = std::get<int>(param[5]);
                                if (param.size() == 7) p.tankIDName = std::get<std::string>(param[6]);
                                SendGamePacket(to, p);
                            }

                            return false;
                        }
                    }
                    else if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC)) LOG_ERROR("param[0] is not std::string, type: {}", param[0].index());
                }
            }
            else {
                if (!ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_PLAYER_MOVING)) return true;

                const BYTE* payload = get_packet_payload(packet);
                if (!payload) return true;
                PlayerMovingView pm(payload);

                switch (pm.PacketType()) {
                    case Network.PACKET_STATE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_CALL_FUNCTION: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_UPDATE_STATUS: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_TILE_CHANGE_REQUEST: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_MAP_DATA: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_TILE_UPDATE_DATA: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_TILE_ACTIVATE_REQUEST: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_TILE_APPLY_DAMAGE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_INVENTORY_STATE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_ITEM_ACTIVATE_REQUEST: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_ITEM_ACTIVATE_OBJECT_REQUEST: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_TILE_TREE_STATE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_MODIFY_ITEM_INVENTORY: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_ITEM_CHANGE_OBJECT: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_LOCK: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_ITEM_DATABASE_DATA: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_PARTICLE_EFFECT: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SET_ICON_STATE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_ITEM_EFFECT: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SET_CHARACTER_STATE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_PING_REPLY: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_PING_REQUEST: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_GOT_PUNCHED: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_APP_CHECK_RESPONSE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_APP_INTEGRITY_FAIL: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_DISCONNECT: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_BATTLE_JOIN: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_BATTLE_EVENT: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_USE_DOOR: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_PARENTAL: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_GONE_FISHIN: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_STEAM: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_PET_BATTLE: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_NPC: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SPECIAL: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_PARTICLE_EFFECT_V2: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_ACTIVE_ARROW_TO_ITEM: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SELECT_TILE_INDEX: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SEND_PLAYER_TRIBUTE_DATA: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_PVE_UNK1:
                    case Network.PACKET_PVE_UNK2:
                    case Network.PACKET_PVE_UNK3:
                    case Network.PACKET_PVE_UNK4:
                    case Network.PACKET_PVE_UNK5:
                    {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_SET_EXTRA_MODS: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    case Network.PACKET_ON_STEP_ON_TILE_MOD: {
                        LOG_INFO("[{}] [{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), BuildPlayerMovingLog(pm));
                        return true;
                    }
                    default: {
                        LOG_ERROR("[{}] [{}] [{} {}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), pm.PacketType(), BuildPlayerMovingLog(pm));
                        return true;

                        /*
                            [16:02:2026 14:24:00] [ERROR] [NETWORK] [SERVER -> PROXY -> CLIENT] [NET_MESSAGE_GAME_PACKET] [PACKET_UNKNOWN -2139095020]
                            NetID:1|X:1200.00|Y:200.00|XSpeed:250.00|YSpeed:1000.00|

                            --------------------------------------------------------------------------------------------------------------------------

                            @note: Client send this packet every 2 minutes, maybe something like ping pong

                            [16:02:2026 14:34:03] [ERROR] [NETWORK] [CLIENT -> PROXY -> SERVER] [NET_MESSAGE_GAME_PACKET] [PACKET_UNKNOWN 65557]
                            PlantingTree:124937|X:64.00|Y:64.00|XSpeed:1000.00|YSpeed:250.00|
                        */
                    }
                }
            }

            break;
        }

        case Network.NET_MESSAGE_ERROR: {
            /* @info: Don't edit this packet */
            std::string text = GetPacketText(packet);
            if (ShouldLogNetMessage(Type)) LOG_WARN("[{}] [{}]\n{}\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text, GetPacketHex(packet));
            return true;
        }
        case Network.NET_MESSAGE_TRACK: {
            /* @info: Don't edit this packet */
            std::string text = GetPacketText(packet);
            if (ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);
            return true;
        }

        case Network.NET_MESSAGE_CLIENT_LOG_REQUEST: {
            /* @info: Don't edit this packet */
            std::string text = GetPacketText(packet);
            if (ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);
            return true;
        }
        case Network.NET_MESSAGE_CLIENT_LOG_RESPONSE: {
            /* @info: Don't edit this packet */
            std::string text = GetPacketText(packet);
            if (ShouldLogNetMessage(Type)) LOG_WARN("[{}] [{}]\n{}\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text, GetPacketHex(packet));
            return true;
        }

        case Network.NET_MESSAGE_UNKNOWN:
        default:
        {
            /* @info: We don't know what is this */
            std::string text = GetPacketText(packet);
            if (ShouldLogNetMessage(Network.NET_MESSAGE_UNKNOWN)) LOG_ERROR("[{}] [{}]\n{}\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text, GetPacketHex(packet));
            return true;
        }
    }

    return true;
}