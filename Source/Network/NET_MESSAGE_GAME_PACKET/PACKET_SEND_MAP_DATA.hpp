#pragma once

#include "../Network.hpp"
#include "../../Main/Config.hpp"
#include "../../Packets/Packets.hpp"
#include "../../Logger/Logger.hpp"
#include "../../Functions/Functions.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>

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

inline void handle_unknown_type(const uint8_t*& ptr, uint8_t type, const uint8_t* buffer_end) {
    auto safe_advance = [&](size_t n) {
        if (ptr + n <= buffer_end) ptr += n;
        else ptr = buffer_end;
    };

    switch (type) {
        case 14: safe_advance(23); break;
        case 19: safe_advance(18); break;
        case 27: safe_advance(4); break;
        case 30: safe_advance(5); break;
        case 35: {
            if (ptr + 2 <= buffer_end) {
                uint16_t len = *(uint16_t*)ptr;
                safe_advance(4 + len);
            }
        } break;
        case 39: safe_advance(4); break;
        case 42: safe_advance(21); break;
        case 43: safe_advance(16); break;
        case 48: {
            if (ptr + 2 <= buffer_end) {
                uint16_t len = *(uint16_t*)ptr;
                safe_advance(26 + len);
            }
        } break;
        case 49: safe_advance(9); break;
        case 55: {
            if (ptr + 4 <= buffer_end) {
                uint32_t count = *(uint32_t*)ptr;
                safe_advance(4 + count * 4 + 16);
            }
        } break;
        case 56: safe_advance(4); break;
        case 58: break;
        case 61: safe_advance(35); break;
        case 62: safe_advance(14); break;
        case 63: {
            if (ptr + 4 <= buffer_end) {
                uint32_t count = *(uint32_t*)ptr;
                safe_advance(4 + count * 15 + 8);
            }
        } break;
        case 65: safe_advance(17); break;
        case 66: safe_advance(1); break;
        case 69: safe_advance(16); break;
        case 71: safe_advance(44); break;
        case 72: safe_advance(12); break;
        case 73: safe_advance(4); break;
        case 74: break;
        case 75: safe_advance(3 * 104); break;
        case 77: {
            if (ptr + 4 <= buffer_end) {
                uint32_t count = *(uint32_t*)ptr;
                safe_advance(4 + count * 4);
            }
        } break;
        case 81: safe_advance(8); break;
        default: break;
    }
}

inline void parse_tile(const uint8_t*& ptr, const uint8_t* buffer_end, int x, int y) {
    if (ptr + 8 > buffer_end) return;

    Tile tile{};
    tile.fg = *(uint16_t*)ptr; ptr += 2;
    tile.bg = *(uint16_t*)ptr; ptr += 2;
    tile.parent = *(uint16_t*)ptr; ptr += 2;
    tile.flag = *(uint16_t*)ptr; ptr += 2;

    // LOCK flag kontrolü
    if ((tile.flag & (1 << 1)) != 0) { // LOCK
        if (ptr + 2 > buffer_end) return;
        tile.lock_parent = *(uint16_t*)ptr; ptr += 2;
    }

    // Basit debug çıktısı
    if ((tile.fg != 0 && tile.bg != 0) && tile.fg != 2 && tile.fg != 4 && tile.fg != 8 && tile.fg != 10 && tile.fg != 14) {
        LOG_DEBUG("Tile ({},{}) FG:{} BG:{} Flags:{}", x, y, tile.fg, tile.bg, tile.flag);
    }

    // EXTRA flag kontrolü
    if ((tile.flag & (1 << 0)) != 0) { // EXTRA
        if (ptr + 1 > buffer_end) return;
        tile.extra.type = *ptr; ptr += 1;
        LOG_DEBUG("TileExtra Type: {}", (int)tile.extra.type);

        // DonationBox veya container blok
        if (1 == 2) {
            if (ptr + 4 > buffer_end) return;
            uint32_t item_count = *(uint32_t*)ptr; ptr += 4;

            std::vector<uint32_t> items(item_count);
            if (ptr + item_count * 4 > buffer_end) return;

            for (uint32_t i = 0; i < item_count; i++) {
                items[i] = *(uint32_t*)ptr; ptr += 4;
            }
            LOG_DEBUG("    DonationBox FG:{} items:{}", tile.fg, item_count);
        }
        else {
            switch (tile.extra.type) {
            case 1: { // Door
                if (ptr + 2 <= buffer_end) {
                    uint16_t len = *(uint16_t*)ptr; ptr += 2;
                    if (ptr + len + 1 <= buffer_end) {
                        std::string label((char*)ptr, len); ptr += len;
                        uint8_t unk = *ptr; ptr += 1;
                        LOG_DEBUG("    Door label:{} unk:{} FG:{} BG:{}", label, unk, tile.fg, tile.bg);
                    }
                }
            } break;

            case 2: { // Sign
                if (ptr + 2 <= buffer_end) {
                    uint16_t len = *(uint16_t*)ptr; ptr += 2;
                    if (ptr + len + 4 <= buffer_end) {
                        std::string label((char*)ptr, len); ptr += len;
                        uint32_t unk = *(uint32_t*)ptr; ptr += 4;
                        LOG_DEBUG("    Sign label:{} unk:{} FG:{} BG:{}", label, unk, tile.fg, tile.bg);
                    }
                }
            } break;

            case 3: { // Lock
                if (ptr + 9 <= buffer_end) {
                    ptr += 1;
                    uint32_t owner = *(uint32_t*)ptr; ptr += 4;
                    uint32_t access_count = *(uint32_t*)ptr; ptr += 4;
                    if (ptr + access_count * 4 + 8 <= buffer_end) {
                        std::vector<uint32_t> accesses(access_count);
                        for (uint32_t i = 0; i < access_count; i++) {
                            accesses[i] = *(uint32_t*)ptr; ptr += 4;
                        }
                        ptr += 8; // extra skip
                        LOG_DEBUG("    Lock owner_id:{} accesses:{} FG:{} BG:{}", owner, access_count, tile.fg, tile.bg);
                    }
                }
            } break;

            case 4: { // Seed
                if (ptr + 5 <= buffer_end) {
                    uint32_t growth = *(uint32_t*)ptr; ptr += 4;
                    uint8_t fruit = *ptr; ptr += 1;
                    LOG_DEBUG("    Seed growth:{} fruit:{} FG:{} BG:{}", growth, fruit, tile.fg, tile.bg);
                }
            } break;

            case 5: { // Dice
                if (ptr + 1 <= buffer_end) {
                    uint8_t number = *ptr; ptr += 1;
                    LOG_DEBUG("    Dice number:{} FG:{} BG:{}", number, tile.fg, tile.bg);
                }
            } break;

            case 6: { // Provider
                if (ptr + 8 <= buffer_end) {
                    uint32_t unk1 = *(uint32_t*)ptr; ptr += 4;
                    uint32_t unk2 = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    Provider unk1:{} unk2:{} FG:{} BG:{}", unk1, unk2, tile.fg, tile.bg);
                }
            } break;

            case 7: { // Achievement
                if (ptr + 5 <= buffer_end) {
                    uint32_t unk1 = *(uint32_t*)ptr; ptr += 4;
                    uint8_t unk2 = *ptr; ptr += 1;
                    LOG_DEBUG("    Achievement unk1:{} unk2:{} FG:{} BG:{}", unk1, unk2, tile.fg, tile.bg);
                }
            } break;

            case 8: { // HeartMonitor
                if (ptr + 6 <= buffer_end) {
                    uint32_t unk = *(uint32_t*)ptr; ptr += 4;
                    uint16_t len = *(uint16_t*)ptr; ptr += 2;
                    if (ptr + len <= buffer_end) {
                        std::string label((char*)ptr, len); ptr += len;
                        LOG_DEBUG("    HeartMonitor unk:{} label:{} FG:{} BG:{}", unk, label, tile.fg, tile.bg);
                    }
                }
            } break;

            case 9: { // BunnyEgg
                if (ptr + 4 <= buffer_end) {
                    uint32_t unk = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    BunnyEgg unk:{} FG:{} BG:{}", unk, tile.fg, tile.bg);
                }
            } break;

            case 10: { // GameGen
                if (ptr + 1 <= buffer_end) {
                    uint8_t unk = *ptr; ptr += 1;
                    LOG_DEBUG("    GameGen unk:{} FG:{} BG:{}", unk, tile.fg, tile.bg);
                }
            } break;

            case 11: { // Xenonite
                if (ptr + 5 <= buffer_end) {
                    uint8_t unk1 = *ptr; ptr += 1;
                    uint32_t unk2 = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    Xenonite unk1:{} unk2:{} FG:{} BG:{}", unk1, unk2, tile.fg, tile.bg);
                }
            } break;

            case 12: { // Crystal
                if (ptr + 2 <= buffer_end) {
                    uint16_t len = *(uint16_t*)ptr; ptr += 2;
                    if (ptr + len <= buffer_end) {
                        std::string unk((char*)ptr, len); ptr += len;
                        LOG_DEBUG("    Crystal unk:{} FG:{} BG:{}", unk, tile.fg, tile.bg);
                    }
                }
            } break;

            case 13: { // Burglar
                if (ptr + 7 <= buffer_end) {
                    uint16_t len = *(uint16_t*)ptr; ptr += 2;
                    if (ptr + len + 5 <= buffer_end) {
                        std::string unk((char*)ptr, len); ptr += len;
                        uint32_t unk2 = *(uint32_t*)ptr; ptr += 4;
                        uint8_t unk3 = *ptr; ptr += 1;
                        LOG_DEBUG("    Burglar unk:{} unk2:{} unk3:{} FG:{} BG:{}", unk, unk2, unk3, tile.fg, tile.bg);
                    }
                }
            } break;

            case 14: { // DisplayBlock
                if (ptr + 4 <= buffer_end) {
                    uint32_t item = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    DisplayBlock item:{} FG:{} BG:{}", item, tile.fg, tile.bg);
                }
            } break;

            case 15: { // Vending
                if (ptr + 8 <= buffer_end) {
                    uint32_t item = *(uint32_t*)ptr; ptr += 4;
                    uint32_t price = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    Vending item:{} price:{} FG:{} BG:{}", item, price, tile.fg, tile.bg);
                }
            } break;

            case 16: { // Solar
                if (ptr + 8 <= buffer_end) {
                    uint32_t unk1 = *(uint32_t*)ptr; ptr += 4;
                    uint32_t count = *(uint32_t*)ptr; ptr += 4;
                    if (ptr + count * 4 <= buffer_end) ptr += count * 4;
                    LOG_DEBUG("    Solar unk1:{} count:{} FG:{} BG:{}", unk1, count, tile.fg, tile.bg);
                }
            } break;

            case 17: { // Deco
                if (ptr + 12 <= buffer_end) {
                    uint32_t unk1 = *(uint32_t*)ptr; ptr += 4;
                    uint32_t unk2 = *(uint32_t*)ptr; ptr += 4;
                    uint32_t unk3 = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    Deco unk1:{} unk2:{} unk3:{} FG:{} BG:{}", unk1, unk2, unk3, tile.fg, tile.bg);
                }
            } break;

            case 18: { // SewingMachine
                if (ptr + 4 <= buffer_end) {
                    uint32_t unk = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    SewingMachine unk:{} FG:{} BG:{}", unk, tile.fg, tile.bg);
                }
            } break;

            case 19: { // CountryFlag
                if (ptr + 2 <= buffer_end) {
                    uint16_t flag_id = *(uint16_t*)ptr; ptr += 2;
                    LOG_DEBUG("    CountryFlag flag_id:{} FG:{} BG:{}", flag_id, tile.fg, tile.bg);
                }
            } break;

            case 20: { // BattleCage
                if (ptr + 14 <= buffer_end) {
                    uint16_t label_len = *(uint16_t*)ptr; ptr += 2;
                    if (ptr + label_len + 12 <= buffer_end) {
                        std::string label((char*)ptr, label_len); ptr += label_len;
                        uint32_t pet1 = *(uint32_t*)ptr; ptr += 4;
                        uint32_t pet2 = *(uint32_t*)ptr; ptr += 4;
                        uint32_t pet3 = *(uint32_t*)ptr; ptr += 4;
                        LOG_DEBUG("    BattleCage label:{} pets:{} {} {} FG:{} BG:{}", label, pet1, pet2, pet3, tile.fg, tile.bg);
                    }
                }
            } break;

            case 21: { // WeatherSpecial
                if (ptr + 4 <= buffer_end) {
                    uint32_t color = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    WeatherSpecial color:{} FG:{} BG:{}", color, tile.fg, tile.bg);
                }
            } break;

            case 22: { // VipEntrance
                if (ptr + 9 <= buffer_end) {
                    ptr += 1;
                    uint32_t owner = *(uint32_t*)ptr; ptr += 4;
                    uint32_t access_count = *(uint32_t*)ptr; ptr += 4;
                    if (ptr + access_count * 4 <= buffer_end) ptr += access_count * 4;
                    LOG_DEBUG("    VipEntrance owner:{} access_count:{} FG:{} BG:{}", owner, access_count, tile.fg, tile.bg);
                }
            } break;

            case 23: { // GeigerCharger
                if (ptr + 4 <= buffer_end) {
                    uint32_t unk = *(uint32_t*)ptr; ptr += 4;
                    LOG_DEBUG("    GeigerCharger unk:{} FG:{} BG:{}", unk, tile.fg, tile.bg);
                }
            } break;

            default: { // Unknown types
                    handle_unknown_type(ptr, tile.extra.type, buffer_end);
                    //LOG_DEBUG("    Unknown TileExtra type:{} FG:{} BG:{}", (int)extra.type, fg, bg);
                    break;
                }
            }
        }
    }
}

inline void parse_world(const uint8_t* data, size_t size, int xSize, int ySize, int block_count) {
    const uint8_t* ptr = data;
    const uint8_t* buffer_end = data + size;

    for (int i = 0; i < block_count; i++) {
        int x = i % xSize;
        int y = i / xSize;
        parse_tile(ptr, buffer_end, x, y);
        if (ptr >= buffer_end) break;
    }
}

class PACKET_SEND_MAP_DATA {
public:
    static inline bool Inject(std::string Method, int Type, ENetPacket* packet,
        ENetPeer* CLIENT, ENetPeer* SERVER, const PlayerMovingView& pm) {
        
        return true; // @todo: fix everything

        if (Type != 4 || !packet || !packet->data) return true;

        const uint8_t* data = packet->data;
        size_t packet_size = packet->dataLength;

        if (packet_size < 80) return true;

        uint8_t name_len = *(uint8_t*)(data + 66);
        std::string world_name((char*)(data + 68), name_len);

        uint8_t xSize = *(uint8_t*)(data + 68 + name_len);
        uint8_t ySize = *(uint8_t*)(data + 72 + name_len);
        uint16_t block_count = *(uint16_t*)(data + 76 + name_len);

        LOG_DEBUG("World Name: {}", world_name);
        LOG_DEBUG("World Size: {} x {}", xSize, ySize);
        LOG_DEBUG("Total Blocks: {}", block_count);

        const uint8_t* blc = data + 80 + name_len;
        if (blc >= data + packet_size) return true;
        blc += 5; // server extra skip

        parse_world(blc, packet_size - (blc - data), xSize, ySize, block_count);

        return true;
    }
};

/* @debug
[24:02:2026 20:07:19] [DEBUG] [NETWORK] World Name: GTSOXOIDSS
[24:02:2026 20:07:19] [DEBUG] [NETWORK] World Size: 100 x 60
[24:02:2026 20:07:19] [DEBUG] [NETWORK] Total Blocks: 6000
[24:02:2026 20:07:19] [DEBUG] [NETWORK] TileExtra Type: 3
[24:02:2026 20:07:19] [DEBUG] [NETWORK]     Lock owner_id:184573809 accesses:0 FG:242 BG:0
[24:02:2026 20:07:19] [DEBUG] [NETWORK] TileExtra Type: 2
[24:02:2026 20:07:19] [DEBUG] [NETWORK]     Sign label: unk:4294967295 FG:1452 BG:0
[24:02:2026 20:07:19] [DEBUG] [NETWORK] TileExtra Type: 4
[24:02:2026 20:07:19] [DEBUG] [NETWORK]     Seed growth:58 fruit:2 FG:15 BG:0
[24:02:2026 20:07:19] [DEBUG] [NETWORK] TileExtra Type: 4
[24:02:2026 20:07:19] [DEBUG] [NETWORK]     Seed growth:58 fruit:4 FG:15 BG:0
[24:02:2026 20:07:19] [DEBUG] [NETWORK] TileExtra Type: 4
[24:02:2026 20:07:19] [DEBUG] [NETWORK]     Seed growth:58 fruit:2 FG:15 BG:0
[24:02:2026 20:07:19] [DEBUG] [NETWORK] TileExtra Type: 4
[24:02:2026 20:07:19] [DEBUG] [NETWORK]     Seed growth:58 fruit:3 FG:3 BG:0
[24:02:2026 20:07:19] [DEBUG] [NETWORK] TileExtra Type: 1
[24:02:2026 20:07:19] [DEBUG] [NETWORK]     Door label:EXIT unk:0 FG:6 BG:0
[24:02:2026 20:07:19] [DEBUG] [NETWORK] Tile (1,59) FG:3760 BG:14 Flags:1
[24:02:2026 20:07:19] [DEBUG] [NETWORK] TileExtra Type: 42 
*/