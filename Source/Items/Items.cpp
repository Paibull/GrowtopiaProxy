#pragma warning(push)
#pragma warning(disable : 4018)

#include "Items.hpp"
#include "../Logger/Logger.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

gItems Items;

std::string getGrowtopiaItemsPath() {
    if (auto* localAppData = std::getenv("LOCALAPPDATA")) return (std::filesystem::path(localAppData) / "Growtopia" / "cache" / "items.dat").string();
    return {};
}


std::string gItems::Inject() {
    std::string path = getGrowtopiaItemsPath();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return "Cannot open file: " + path + "\n";

    auto size = file.tellg();
    if (size <= 0) return "File size error\n";
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) return "Failed to read file\n";

    packetData.resize(60 + size);
    std::memset(packetData.data(), 0, 60);

    int32_t MessageType = 0x4, PacketType = 0x10, NetID = -1, CharState = 0x8;
    std::memcpy(packetData.data(), &MessageType, 4);
    std::memcpy(packetData.data() + 4, &PacketType, 4);
    std::memcpy(packetData.data() + 8, &NetID, 4);
    std::memcpy(packetData.data() + 16, &CharState, 4);
    std::memcpy(packetData.data() + 56, &size, 4);
    std::copy(data.begin(), data.end(), packetData.begin() + 60);

    unsigned char* _Data = data.data();
    std::size_t pos = 0;

    uint8_t itemsdatVersion = _Data[pos];
    pos += 2;
    uint32_t itemCount = *reinterpret_cast<uint32_t*>(_Data + pos);
    pos += 4;

    if (itemsdatVersion < 11 || itemsdatVersion > 24) return "Your items.dat version is not supported [" + std::to_string(itemsdatVersion) + "]\n";

    items.reserve(itemCount);

    auto read_int = [&_Data, &pos](void* dst, std::size_t len) {
        std::memcpy(dst, _Data + pos, len);
        pos += len;
    };
    auto read_int_safe = [&_Data, &pos, size](void* dst, std::size_t len) {
        if (pos + static_cast<std::size_t>(len) > size) { pos = size; return; }
        std::memcpy(dst, _Data + pos, len);
        pos += len;
    };
    auto read_str = [&_Data, &pos, size](std::string& ref) {
        if (pos + 2 > size) { ref.clear(); return; }
        uint16_t len = *reinterpret_cast<uint16_t*>(_Data + pos);
        pos += 2;
        if (pos + static_cast<std::size_t>(len) > size) { ref.clear(); pos = size; return; }
        ref.clear();
        ref.reserve(len);
        for (std::size_t i = 0; i < static_cast<std::size_t>(len); i++) ref += _Data[pos++];
    };
    auto read_str_chiped = [&_Data, &pos, size](std::string& ref, uint32_t id) {
        if (pos + 2 > size) { ref.clear(); return; }
        uint16_t len = *reinterpret_cast<uint16_t*>(_Data + pos);
        pos += 2;
        if (pos + static_cast<std::size_t>(len) > size) { ref.clear(); pos = size; return; }
        constexpr std::string_view key = "PBG892FXX982ABC*";
        ref.clear();
        ref.reserve(len);
        for (std::size_t i = 0; i < static_cast<std::size_t>(len); i++) ref += _Data[pos++] ^ key[(i + id) % key.size()];
    };

    itemID.clear();
    items.clear();

    int real_id = 0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(itemCount); i++) {
        ItemDefinition def;

        memcpy(&def.Id, _Data + pos, 4);
        pos += 4;
        real_id = def.Id;

        read_int(&def.Editable_Type, 1);
        read_int(&def.Item_Category, 1);
        read_int(&def.Material_Type, 1);
        read_int(&def.Hit_Sound_Type, 1);

        read_str_chiped(def.Name, def.Id);
        read_str(def.Texture);
        read_int(&def.Texture_Hash, 4);

        read_int(&def.Visual_Effect_Type, 1);
        read_int(&def.Cooking_Time, 4);

        read_int(&def.Texture_X, 1);
        read_int(&def.Texture_Y, 1);

        read_int(&def.Storage_Type, 1);
        read_int(&def.Is_Stripey_Wallpaper, 1);
        read_int(&def.Collision_Type, 1);

        uint8_t tmp_break_hits = 0;
        read_int(&tmp_break_hits, 1);
        def.Break_Hits = tmp_break_hits / 6;

        read_int(&def.Reset_State_After, 4);
        read_int(&def.Clothing_Type, 1);
        read_int(&def.Rarity, 2);
        read_int(&def.Max_Amount, 1);

        read_str(def.Extra_File);
        read_int(&def.Extra_File_Hash, 4);

        read_int(&def.Audio_Volume, 4);

        read_str(def.Pet_Name);
        read_str(def.Pet_Prefix);
        read_str(def.Pet_Suffix);
        read_str(def.Pet_Ability);

        read_int(&def.Seed_Base, 1);
        read_int(&def.Seed_Overlay, 1);
        read_int(&def.Tree_Base, 1);
        read_int(&def.Tree_Leaves, 1);

        read_int(&def.Seed_Color, 4);
        read_int(&def.Seed_Overlay_Color, 4);
        read_int(&def.Ingredient, 4);
        read_int(&def.Grow_Time, 4);

        read_int(&def.Flags2, 2);
        read_int(&def.Rayman, 2);

        read_str(def.Extra_Options);
        read_str(def.Texture2_Path);
        read_str(def.Extra_Options2);

        read_int(def.Unk_Data1, 80);

        if (itemsdatVersion >= 11) read_str(def.Punch_Options);
        if (itemsdatVersion >= 12) { read_int_safe(&def.Flags3, 4); read_int_safe(def.Bodypart, 9); }
        if (itemsdatVersion >= 13) read_int_safe(&def.Flags4, 4);
        if (itemsdatVersion >= 14) read_int_safe(&def.Flags5, 4);
        if (itemsdatVersion >= 15) { read_int_safe(def.Unk_Data2, 25); read_str(def.Texture3_Path); }
        if (itemsdatVersion >= 16) { std::string dummy; read_str(dummy); }
        if (itemsdatVersion >= 17) pos += 4;
        if (itemsdatVersion >= 18) pos += 4;
        if (itemsdatVersion >= 19) pos += 9;
        if (itemsdatVersion >= 21) pos += 2;
        if (itemsdatVersion >= 22) {
            uint16_t strLen = 0;
            read_int_safe(&strLen, 2);
            pos += strLen;
        }
        if (itemsdatVersion >= 23) { read_int_safe(&def.Ingredient1, 2); read_int_safe(&def.Ingredient2, 2); }
        if (itemsdatVersion >= 24) pos += 1;

        itemID[def.Id] = items.size();
        itemNAME[def.Name] = items.size();
        items.push_back(std::move(def));

    }

    LOG_INFO("Loaded {} Items from the Items.dat V{}", itemCount, static_cast<int>(itemsdatVersion));

    return "";
}

const ItemDefinition* gItems::GetItemByID(int32_t id) const {
    auto it = itemID.find(id);
    if (it != itemID.end())
        return &items[it->second];
    return nullptr;

    /*
        if (const ItemDefinition* d = Items.GetItemByID(2)) {
            int id = d->Id;
        }
    */
}

ItemDefinition* gItems::GetItemByID(int32_t id) {
    auto it = itemID.find(id);
    if (it != itemID.end()) return &items[it->second];
    return nullptr;
}

const ItemDefinition* gItems::GetItemByName(const std::string& name) const {
    auto it = itemNAME.find(name);
    if (it != itemNAME.end()) return &items[it->second];
    return nullptr;

    /*
        if (const ItemDefinition* d = Items.GetItemByName("World Lock")) {
            std::string name = d->Name;
            int id = d->Id;
        }
    */
}

ItemDefinition* gItems::GetItemByName(const std::string& name) {
    auto it = itemNAME.find(name);
    if (it != itemNAME.end()) return &items[it->second];
    return nullptr;
}

#pragma warning(pop)