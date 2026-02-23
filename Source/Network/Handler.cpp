#include "Handler.hpp"
#include "Network.hpp"
#include "../Main/Config.hpp"
#include "../Logger/Logger.hpp"
#include "../Functions/Functions.hpp"
#include "../Packets/Packets.hpp"
#include "../Player/Player.hpp"
#include "../Items/Items.hpp"

#include <iomanip>
#include <sstream>
#include <vector>
#include <variant>

#include "NET_MESSAGE_GAME_PACKET/PACKET_STATE.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_CALL_FUNCTION.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_TILE_CHANGE_REQUEST.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_SEND_INVENTORY_STATE.hpp"
#include "NET_MESSAGE_GAME_PACKET/PACKET_MODIFY_ITEM_INVENTORY.hpp"

int GetPacketType(const ENetPacket* packet) {
    if (!packet || packet->dataLength < 1) return -1;
    return *(int*)(packet->data);
}

std::string GetPacketText(ENetPacket* packet) {
    if (!packet) return "Unknown Packet Text";
    char zero = 0;
    memcpy(packet->data + packet->dataLength - 1, &zero, 1);
    return std::string((char*)(packet->data + 4));
}

std::string GetPacketHex(const ENetPacket* packet) {
    if (!packet || packet->dataLength == 0) return "Unknown Packet Hex";
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < packet->dataLength; ++i) oss << std::setw(2) << static_cast<unsigned>(packet->data[i]) << " ";
    return oss.str();
}

void SendPacket(ENetPeer* peer, int num, const char* data, int len) {
    ENetPacket* packet = enet_packet_create(nullptr, len + 5, ENET_PACKET_FLAG_RELIABLE);
    if (!packet) return;
    memcpy(packet->data, &num, 4);
    if (data && len > 0) memcpy(packet->data + 4, data, len);
    packet->data[4 + len] = 0x00;
    enet_peer_send(peer, 0, packet);
    /* SendPacket(event.peer, type, string.c_str(), string.length()); */
}

bool HandlePacket(int type, ENetPacket* packet, ENetPeer* to) {
    /* @important: if you want to forward the packet without editing (the original) then use 'return true;'
        if you want to edit, after editing the packet, use 'return false;' so it won't send the original packet but your custom packet
    */

    if (!packet) return true;

    ENetPeer* CLIENT = nullptr;
    ENetPeer* SERVER = nullptr;

    std::string Method;
    if (type == Network.CLIENT_PROXY_SERVER) {
        Method = "CLIENT -> PROXY -> SERVER";
        auto it = Network.server_to_client.find(to);
        if (it != Network.server_to_client.end()) {
            SERVER = to;
            CLIENT = it->second;
        }
    }
    else if (type == Network.SERVER_PROXY_CLIENT) {
        Method = "SERVER -> PROXY -> CLIENT";
        auto it = Network.client_to_server.find(to);
        if (it != Network.client_to_server.end()) {
            CLIENT = to;
            SERVER = it->second;
        }
    }

    int Type = GetPacketType(packet);

    switch (Type) {
        case Network.NET_MESSAGE_SERVER_HELLO: {
            /* @info: Don't edit this packet */
            if (ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), GetPacketHex(packet));
            return true;
        }

        case Network.NET_MESSAGE_GENERIC_TEXT: {
            std::string text = GetPacketText(packet);
            if (ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);

            /* @info: Edit here */

            if (Packet::Contains(text, "action|enter_game") && !Packet::Contains(text, "action|enter_game\ninvitedWorld|")) {
                std::string ItemsLog = Items.Inject();
                if (!ItemsLog.empty()) LOG_ERROR("{}", ItemsLog);
            }
            else if (Packet::ContainsValue<std::string>(text, "action")) {
                std::string action = Packet::ExtractValue<std::string>(text, "action", "");

                if (action == "input") {
                    std::string command = Packet::ExtractValue<std::string>(text, "text", "");
                    
                    if (!command.empty() && command[0] == '/') {
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
                    }
                }
            }

            return true;
        }

        case Network.NET_MESSAGE_GAME_MESSAGE: {
            std::string text = GetPacketText(packet);
            if (ShouldLogNetMessage(Type)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), text);

            /* @info: Edit here */

            return true;
        }

        case Network.NET_MESSAGE_GAME_PACKET: {
            size_t payloadSize = 0;
            const BYTE* payload = get_packet_payload(packet, payloadSize);
            if (!payload) return true;

            PlayerMovingView pm(payload, payloadSize);

            if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_PLAYER_MOVING)) LOG_INFO("[{}] [{}] [{}:{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), Network.PLAYER_MOVING_STRING(pm.PacketType()), pm.PacketType(), BuildPlayerMovingLog(pm));

            switch (pm.PacketType()) {
                case Network.PACKET_STATE: {
                    return PACKET_STATE::Inject(Method, Type, packet, CLIENT, SERVER, pm);
                }
                case Network.PACKET_CALL_FUNCTION: {
                    return PACKET_CALL_FUNCTION::Inject(Method, Type, packet, CLIENT, SERVER);
                }
                case Network.PACKET_TILE_CHANGE_REQUEST: {
                    return PACKET_TILE_CHANGE_REQUEST::Inject(Method, Type, packet, CLIENT, SERVER, pm);
                }
                case Network.PACKET_SEND_INVENTORY_STATE: {
                    return PACKET_SEND_INVENTORY_STATE::Inject(Method, Type, packet, CLIENT, SERVER);
                }
                case Network.PACKET_MODIFY_ITEM_INVENTORY: {
                    return PACKET_MODIFY_ITEM_INVENTORY::Inject(Method, Type, packet, CLIENT, SERVER, pm);
                }
                default: {
                    return true;
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