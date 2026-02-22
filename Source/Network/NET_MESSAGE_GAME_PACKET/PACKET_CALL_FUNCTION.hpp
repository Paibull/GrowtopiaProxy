#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <variant>
#include <vector>
#include <cstring>

class PACKET_CALL_FUNCTION {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet, ENetPeer* CLIENT, ENetPeer* SERVER) {
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

                uint8_t idx = packet->data[cursor];
                uint8_t vtype = packet->data[cursor + 1];
                cursor += 2;
                readCount++;

                switch (vtype) {

                case 0x2: { // STRING
                    if (cursor + sizeof(int) > packet->dataLength) return true;

                    int slen = ReadInt(packet->data, cursor);
                    if (slen < 0 || cursor + slen > packet->dataLength) return true;

                    std::string value((char*)packet->data + cursor, slen);
                    cursor += slen;

                    debug += "[" + std::to_string(idx) + "][STRING] " + value + "\n";
                    param.push_back(value);
                    break;
                }

                case 0x1: { // FLOAT
                    if (cursor + sizeof(float) > packet->dataLength) return true;

                    float v = ReadFloat(packet->data, cursor);
                    debug += "[" + std::to_string(idx) + "][FLOAT] " + std::to_string(v) + "\n";
                    param.push_back(v);
                    break;
                }

                case 0x3: { // Vec2
                    if (cursor + sizeof(float) * 2 > packet->dataLength) return true;

                    Vec2 custom;
                    custom.x = ReadFloat(packet->data, cursor);
                    custom.y = ReadFloat(packet->data, cursor);

                    debug += "[" + std::to_string(idx) + "][FLOAT2] "
                        + std::to_string(custom.x) + ", "
                        + std::to_string(custom.y) + "\n";

                    param.push_back(custom);
                    break;
                }

                case 0x4: { // Vec3
                    if (cursor + sizeof(float) * 3 > packet->dataLength) return true;

                    Vec3 custom;
                    custom.x = ReadFloat(packet->data, cursor);
                    custom.y = ReadFloat(packet->data, cursor);
                    custom.z = ReadFloat(packet->data, cursor);

                    debug += "[" + std::to_string(idx) + "][FLOAT3] "
                        + std::to_string(custom.x) + ", "
                        + std::to_string(custom.y) + ", "
                        + std::to_string(custom.z) + "\n";

                    param.push_back(custom);
                    break;
                }

                case 0x5: { // UINT
                    if (cursor + sizeof(unsigned int) > packet->dataLength) return true;

                    unsigned int v = ReadUInt(packet->data, cursor);
                    debug += "[" + std::to_string(idx) + "][UINT] " + std::to_string(v) + "\n";
                    param.push_back(v);
                    break;
                }

                case 0x6: { // INT64
                    if (cursor + sizeof(long long) > packet->dataLength) return true;

                    long long v = ReadInt64(packet->data, cursor);
                    debug += "[" + std::to_string(idx) + "][INT64] " + std::to_string(v) + "\n";
                    param.push_back(v);
                    break;
                }

                case 0x9: { // INT
                    if (cursor + sizeof(int) > packet->dataLength) return true;

                    int v = ReadInt(packet->data, cursor);
                    debug += "[" + std::to_string(idx) + "][INT] " + std::to_string(v) + "\n";
                    param.push_back(v);
                    break;
                }

                default:
                    return true;
                }
            }
        }

        if (!debug.empty()) {
            if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC)) LOG_INFO("[{}] [{}]\n{}\n", Method, Network.RECEIVE_TYPE_STRING(Type), debug);

            /* @info: Edit here  */
            if (!param.empty()) {
                if (auto pStr = std::get_if<std::string>(&param[0])) {
                    if (std::get<std::string>(param[0]) == "OnDialogRequest" && param.size() == 2) {
                        std::string dialog = std::get<std::string>(param[1]);

                        if (Player.GetDropFlag() &&
                            Packet::Contains(dialog, "add_label_with_icon|big|`wDrop ") &&
                            Packet::Contains(dialog, "add_textbox|How many to drop?|left|")
                            ) {
                            Player.SetDropFlag(false); return false;
                        }

                        GamePacket<OnDialogRequest> p;
                        p.text = std::get<std::string>(param[1]);
                        SendGamePacket(CLIENT, p);

                        return false;
                    }
                    else if (std::get<std::string>(param[0]) == "OnConsoleMessage" && param.size() == 2) {
                        GamePacket<OnConsoleMessage> p;
                        p.text = std::get<std::string>(param[1]);
                        SendGamePacket(CLIENT, p);
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

                        if (SERVER) enet_peer_disconnect_now(SERVER, 0);

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
                            SendGamePacket(CLIENT, p);
                        }

                        return false;
                    }
                    else if (std::get<std::string>(param[0]) == "OnSpawn" && param.size() == 2) {
                        /* @debug:
                            [0][STRING] OnSpawn
                            [1][STRING] spawn|avatar
                            netID|1
                            userID|1845574839
                            eid|12345|4C5OyfGMVMhYShoJDGHIHfFUH73pOOWf680FpmGDXFs=|yMmQHaAODSGcC0nsyVlA==
                            ip|ar7JmdEY5ZMGHWWuofSDGG4RtVGftGkeE9NuzJZ8X4=
                            colrect|0|0|20|30
                            posXY|1440|736
                            name|`2GrowtopiaProxy``
                            titleIcon|{"PlayerWorldID":1,"WrenchCustomization":{"WrenchForegroundCanRotate":false,"WrenchForegroundID":-1,"WrenchIconID":-1}}
                            country|us
                            invis|0
                            mstate|0
                            smstate|0
                            onlineID|
                            type|local
                        */

                        if (Packet::ExtractCustom<std::string>(std::get<std::string>(param[1]), "|", 1, "\n") == "avatar" && Packet::Contains(std::get<std::string>(param[1]), "type|local")) {
                            Player.netID = Packet::ExtractCustom<int>(std::get<std::string>(param[1]), "|", 2, "\n");
                            Player.userID = Packet::ExtractCustom<int>(std::get<std::string>(param[1]), "|", 3, "\n");

                            std::string name = Packet::ExtractCustom<std::string>(std::get<std::string>(param[1]), "|", 14, "\n");
                            Player.tankIDName = name.substr(2, name.size() - 4);
                        }

                        return true;
                    }
                }
                else if (ShouldLogNetMessage(Type, Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC)) LOG_ERROR("param[0] is not std::string, type: {}", param[0].index());
            }
        }
        return true;
    }
private:
    static inline int ReadInt(const uint8_t* data, size_t& cursor) {
        int v;
        std::memcpy(&v, data + cursor, sizeof(int));
        cursor += sizeof(int);
        return v;
    }

    static inline float ReadFloat(const uint8_t* data, size_t& cursor) {
        float v;
        std::memcpy(&v, data + cursor, sizeof(float));
        cursor += sizeof(float);
        return v;
    }

    static inline unsigned int ReadUInt(const uint8_t* data, size_t& cursor) {
        unsigned int v;
        std::memcpy(&v, data + cursor, sizeof(unsigned int));
        cursor += sizeof(unsigned int);
        return v;
    }

    static inline long long ReadInt64(const uint8_t* data, size_t& cursor) {
        long long v;
        std::memcpy(&v, data + cursor, sizeof(long long));
        cursor += sizeof(long long);
        return v;
    }
};