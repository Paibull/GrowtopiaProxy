#pragma once

#include <cstdint>
#include <cstring>
#include <enet/enet.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <array>

using BYTE = unsigned char;

class GamePacketBuilder {
private:
    int index = 0, len = 0;
    std::vector<BYTE> packet_data;

public:
    GamePacketBuilder(int delay = 0, int NetID = -1);

    GamePacketBuilder& Insert(const std::string& a, int red = 0, int green = 0, int blue = 0, int alpha = 160);
    GamePacketBuilder& Insert(int a);
    GamePacketBuilder& Insert(unsigned int a);
    GamePacketBuilder& Insert(float a);
    GamePacketBuilder& Insert(float a, float b);
    GamePacketBuilder& Insert(float a, float b, float c);
    GamePacketBuilder& Insert(long long int a);

    void CreatePacket(ENetPeer* peer);

private:
    template<typename T>
    GamePacketBuilder& InsertValue(const T& value, BYTE type);
};

enum : uint8_t {
    OnConsoleMessage = 1,
    OnTalkBubble,
    OnTextOverlay,

    OnDialogRequest,
    OnSendToServer,
};

inline const char* PacketName(uint8_t type) {
    switch (type) {
        case OnConsoleMessage:                       return "OnConsoleMessage";
        case OnTalkBubble:                           return "OnTalkBubble";
        case OnTextOverlay:                          return "OnTextOverlay";

        case OnDialogRequest:                        return "OnDialogRequest";
        case OnSendToServer:                         return "OnSendToServer";

        default:                                     return "Unknown";
    }
}

template<uint8_t T>
struct GamePacket;

template<> struct GamePacket<OnConsoleMessage> {
    static constexpr uint8_t type = OnConsoleMessage;
    std::string text;
};
template<> struct GamePacket<OnTalkBubble> {
    static constexpr uint8_t type = OnTalkBubble;
    int userId;
    std::string text;
    int unknown = 0;
    int unknown2 = 1;
};
template<> struct GamePacket<OnTextOverlay> {
    static constexpr uint8_t type = OnTextOverlay;
    std::string text;
};

template<> struct GamePacket<OnDialogRequest> {
    static constexpr uint8_t type = OnDialogRequest;
    std::string text;
};
template<> struct GamePacket<OnSendToServer> {
    static constexpr uint8_t type = OnSendToServer;
    int port;
    int token;
    int userId;
    std::string ip;
    std::string doorID;
    std::string UUIDToken;
    int lmode;
    std::string tankIDName;
};


class GamePacketBuilder;

inline void Serialize(GamePacketBuilder& p, const GamePacket<OnConsoleMessage>& v) { p.Insert(v.text); }
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnTalkBubble>& v) { p.Insert(v.userId).Insert(v.text).Insert(v.unknown).Insert(v.unknown2); }
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnTextOverlay>& v) { p.Insert(v.text); }

inline void Serialize(GamePacketBuilder& p, const GamePacket<OnDialogRequest>& v) { p.Insert(v.text); }
inline void Serialize(GamePacketBuilder& p, const GamePacket<OnSendToServer>& v) { p.Insert(v.port).Insert(v.token).Insert(v.userId).Insert(v.ip + "|" + v.doorID + "|" + v.UUIDToken).Insert(v.lmode); if (!v.tankIDName.empty()) p.Insert(v.tankIDName); }

template<uint8_t T>
void SendGamePacket(ENetPeer* peer, const GamePacket<T>& pkt) {
    GamePacketBuilder pckt;
    pckt.Insert(PacketName(T));
    Serialize(pckt, pkt);
    pckt.CreatePacket(peer);
}





namespace PMOffset {
    constexpr size_t FLAG_BYTE = 12;
    constexpr size_t EXTRA_SIZE_DWORD = 52; // 13 * 4
    constexpr size_t DELAY = 24;
    constexpr size_t DEATH_FLAG = 56;
}

class PlayerMovingPacket {
public:
    static constexpr size_t SIZE = 56;

    PlayerMovingPacket() { buffer.fill(0); }

    PlayerMovingPacket& PacketType(int v) { write<int>(0x00, v); return *this; }
    PlayerMovingPacket& NetID(int v) { write<int>(0x04, v); return *this; }
    PlayerMovingPacket& CharacterState(int v) { write<int>(0x0C, v); return *this; }
    PlayerMovingPacket& PlantingTree(int v) { write<int>(0x14, v); return *this; }
    PlayerMovingPacket& Position(float x, float y) { write<float>(0x18, x); write<float>(0x1C, y); return *this; }
    PlayerMovingPacket& Speed(float xs, float ys) { write<float>(0x20, xs); write<float>(0x24, ys); return *this; }
    PlayerMovingPacket& Punch(int x, int y) { write<int>(0x2C, x); write<int>(0x30, y); return *this; }
    PlayerMovingPacket& Packet40(int v) { write<int>(0x28, v); return *this; }
    PlayerMovingPacket& SetByte(size_t offset, BYTE v) { buffer[offset] = v; return *this; }

    void Send(ENetPeer* peer, int a1 = 4, int flags = ENET_PACKET_FLAG_RELIABLE, int delay = 0) const {
        send_raw(peer, a1, buffer.data(), SIZE, flags, delay);
    }

    BYTE* Data() { return buffer.data(); }
    const BYTE* Data() const { return buffer.data(); }

private:
    std::array<BYTE, SIZE> buffer;

    template<typename T>
    inline void write(size_t offset, const T& v) { std::memcpy(buffer.data() + offset, &v, sizeof(T)); }

    static void send_raw(ENetPeer* peer, int a1, const void* packetData, size_t packetDataSize, enet_uint32 packetFlag, int delay = 0) {
        if (!peer || !packetData || packetDataSize == 0) return;
        const BYTE* data = static_cast<const BYTE*>(packetData);
        const bool hasExtra = (a1 == 4) && (data[PMOffset::FLAG_BYTE] & 8);
        size_t extraSize = 0;
        if (hasExtra) std::memcpy(&extraSize, data + PMOffset::EXTRA_SIZE_DWORD, sizeof(int));
        const size_t totalSize = 4 + packetDataSize + (hasExtra ? extraSize : 0);
        ENetPacket* p = enet_packet_create(nullptr, totalSize, packetFlag);
        if (!p) return;
        std::memcpy(p->data, &a1, 4);
        std::memcpy(p->data + 4, packetData, packetDataSize);
        if (hasExtra && extraSize > 0) std::memset(p->data + 4 + packetDataSize, 0, extraSize);
        if (delay != 0) {
            constexpr int deathFlag = 0x19;
            std::memcpy(p->data + PMOffset::DELAY, &delay, 4);
            std::memcpy(p->data + PMOffset::DEATH_FLAG, &deathFlag, 4);
        }
        enet_peer_send(peer, 0, p);
    }
};


class PlayerMovingView {
public:
    explicit PlayerMovingView(const BYTE* data) : data_(data) {}

    int PacketType() const { return read<int>(0x00); }
    int NetID() const { return read<int>(0x04); }
    int CharacterState() const { return read<int>(0x0C); }
    int PlantingTree() const { return read<int>(0x14); }
    float X() const { return read<float>(0x18); }
    float Y() const { return read<float>(0x1C); }
    float XSpeed() const { return read<float>(0x20); }
    float YSpeed() const { return read<float>(0x24); }
    int PunchX() const { return read<int>(0x2C); }
    int PunchY() const { return read<int>(0x30); }
    bool HasExtra() const { return data_[PMOffset::FLAG_BYTE] & 8; }

private:
    const BYTE* data_;
    template<typename T>
    inline T read(size_t offset) const { T v; std::memcpy(&v, data_ + offset, sizeof(T)); return v; }
};

inline const BYTE* get_packet_payload(const ENetPacket* packet) {
    if (!packet || packet->dataLength < 60) return nullptr;
    const BYTE* data = packet->data;
    const BYTE* payload = data + 4;
    if (data[16] & 8) {
        int extraSize = 0;
        std::memcpy(&extraSize, data + 56, sizeof(int));
        if (extraSize < 0) return nullptr;
        if (packet->dataLength < 60u + static_cast<size_t>(extraSize)) return nullptr;
    }
    else {
        int zero = 0;
        std::memcpy(const_cast<BYTE*>(data) + 56, &zero, sizeof(int));
    }
    return payload;
}

inline std::string BuildPlayerMovingLog(const PlayerMovingView& pm) {
    std::ostringstream oss;

    if (pm.NetID() != 0)           oss << "NetID:" << pm.NetID() << "|";
    if (pm.CharacterState() != 0)  oss << "CharacterState:" << pm.CharacterState() << "|";
    if (pm.PlantingTree() != 0)    oss << "PlantingTree:" << pm.PlantingTree() << "|";

    if (pm.X() != 0.0f)            oss << "X:" << std::fixed << std::setprecision(2) << pm.X() << "|";
    if (pm.Y() != 0.0f)            oss << "Y:" << std::fixed << std::setprecision(2) << pm.Y() << "|";
    if (pm.XSpeed() != 0.0f)       oss << "XSpeed:" << std::fixed << std::setprecision(2) << pm.XSpeed() << "|";
    if (pm.YSpeed() != 0.0f)       oss << "YSpeed:" << std::fixed << std::setprecision(2) << pm.YSpeed() << "|";

    if (pm.PunchX() != 0)          oss << "PunchX:" << pm.PunchX() << "|";
    if (pm.PunchY() != 0)          oss << "PunchY:" << pm.PunchY() << "|";

    if (pm.HasExtra())             oss << "HasExtra";

    return oss.str();
}