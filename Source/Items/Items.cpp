#include "items.hpp"

#include <filesystem>
#include <fstream>


std::vector<Items> items;
bool ALREADY_INJECTED = false;

struct pos {
    pos(int _x, int _y) : x(_x), y(_y) {}
    pos(float _x, float _y) : x(std::round(_x / 32.0f)), y(std::round(_y / 32.0f)) {}

    int x{ 0 };
    int y{ 0 };

    float f_x() const { return this->x * 32.0f; }
    float f_y() const { return this->y * 32.0f; }

    auto operator<=>(const pos&) const = default;
};

class state {
public:
    int packet_create{ 04 };

    int type{};
    int netid{};
    int uid{}; // @todo understand this better @note so far I think this holds uid value
    int peer_state{};
    float count{}; // @todo understand this better
    int id{}; // @note peer's active hand, so 18 (fist) = punching, 32 (wrench) interacting, ect
    ::pos pos{ 0,0 }; // @note position 1D {x, y}
    std::array<float, 2zu> speed{}; // @note player movement (velocity(x), gravity(y)), higher gravity = smaller jumps
    int idk{};
    ::pos punch{ 0,0 }; // @note punching/placing position 2D {x, y}
    int size{};
};

std::vector<uint8_t> compress_state(const state& s) {
    std::vector<uint8_t> data(sizeof(::state) + 1, 0x00);
    int* _4bit = reinterpret_cast<int*>(data.data());
    float* _4bit_f = reinterpret_cast<float*>(data.data());
    _4bit[0] = s.packet_create;
    _4bit[1] = s.type;
    _4bit[2] = s.netid;
    _4bit[3] = s.uid;
    _4bit[4] = s.peer_state;
    _4bit_f[5] = s.count;
    _4bit[6] = s.id;
    ::pos f_pos = s.pos; // @todo
    _4bit_f[7] = f_pos.f_x();
    _4bit_f[8] = f_pos.f_y();
    _4bit_f[9] = s.speed[0];
    _4bit_f[10] = s.speed[1];
    _4bit[11] = s.idk;
    _4bit[12] = s.punch.x;
    _4bit[13] = s.punch.y;
    _4bit[14] = s.size;
    return data;
}

std::vector<uint8_t> im_data(sizeof(::state)/*inital packet*/ + 1, 0x00);

template<typename T>
void shift_pos(const std::vector<uint8_t>& data, uint32_t& pos, T& value) {
    uint8_t* _1bit = reinterpret_cast<uint8_t*>(&value);
    for (std::size_t i = 0zu; i < sizeof(T); ++i)
        _1bit[i] = data[pos + i];
    pos += sizeof(T);
}

/* have not tested modifying string values··· */
template<typename T>
void data_modify(std::vector<uint8_t>& data, const uint32_t& pos, const T& value) {
    const uint8_t* _1bit = reinterpret_cast<const uint8_t*>(&value);
    for (std::size_t i = 0zu; i < sizeof(T); ++i)
        data[pos + i] = _1bit[i];
}

std::string getGrowtopiaItemsPath() {
    if (auto* localAppData = std::getenv("LOCALAPPDATA")) return (std::filesystem::path(localAppData) / "Growtopia" / "cache" / "items.dat").string();
    return {};
}

std::string InjectItems() {
    const int size = std::filesystem::file_size(getGrowtopiaItemsPath());
    im_data = compress_state(::state{
        .type = 0x10,
        .peer_state = 0x08,
        .size = size
    });

    im_data.resize(im_data.size() + size); // @note resize to fit binary data
    std::ifstream(getGrowtopiaItemsPath(), std::ios::binary).read(reinterpret_cast<char*>(&im_data[sizeof(::state)]), size); // @note the binary data···

    uint32_t pos{ sizeof(::state) };
    uint8_t version{};
    shift_pos(im_data, pos, version); pos += 1; // @note downsize 'version' to 1 bit
    uint16_t count{};
    shift_pos(im_data, pos, count); pos += 2; // @note downside count to 2 bit
    static constexpr std::string_view token{ "PBG892FXX982ABC*" };
    for (uint16_t i = 0; i < count; ++i) {
        Items im{};

        shift_pos(im_data, pos, im.id); pos += 2; // @note downside im.id to 2 bit (short)
        shift_pos(im_data, pos, im.property);

        shift_pos(im_data, pos, im.cat);

        shift_pos(im_data, pos, im.type);
        pos += sizeof(uint8_t);

        short len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);
        im.raw_name.resize(len);
        for (short i = 0; i < len; ++i) im.raw_name[i] = im_data[pos] ^ token[(i + im.id) % token.length()], ++pos;

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(int);
        pos += sizeof(uint8_t);

        shift_pos(im_data, pos, im.ingredient);
        pos += sizeof(uint8_t);
        pos += sizeof(uint8_t);
        pos += sizeof(uint8_t);
        pos += sizeof(uint8_t);

        shift_pos(im_data, pos, im.collision);
        shift_pos(im_data, pos, im.hits);
        if (im.hits != 0) im.hits /= 6; // @note unknown reason behind why break hit is muliplied by 6 then having to divide by 6

        shift_pos(im_data, pos, im.hit_reset);

        if (im.type == type::CLOTHING) {
            uint8_t cloth_type{};
            shift_pos(im_data, pos, im.cloth_type);
        }
        else pos += 1; // @note assign nothing
        if (im.type == type::AURA) im.cloth_type = clothing::ances;
        shift_pos(im_data, pos, im.rarity);

        pos += sizeof(uint8_t);
        {
            len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            std::string audio_directory{};
            audio_directory.assign(reinterpret_cast<char*>(&im_data[pos]), len);
            pos += len;

            if (audio_directory.ends_with(".mp3")) data_modify(im_data, pos, 0); // @todo make it only for IOS
        }
        pos += sizeof(int);

        pos += sizeof(std::array<uint8_t, 4zu>);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(std::array<uint8_t, 16zu>);

        shift_pos(im_data, pos, im.tick);

        pos += sizeof(short);
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(std::array<uint8_t, 80zu>);

        if (version >= 11) // @date February 2019
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 12) // @date October 2020
        {
            pos += sizeof(int);
            pos += sizeof(std::array<uint8_t, 9zu>);
        }
        if (version >= 13) pos += sizeof(int); // @date May 2021
        if (version >= 14) pos += sizeof(int); // @date October 2021
        if (version >= 15)
        {
            pos += sizeof(std::array<uint8_t, 25zu>);
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 16)
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 17) pos += sizeof(int); // @date April 2024
        if (version >= 18) pos += sizeof(int); // @date December 2024
        if (version >= 19) pos += sizeof(std::array<uint8_t, 9zu>);
        if (version >= 21) pos += sizeof(short); // @date September 2025
        if (version >= 22)
        {
            len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            im.info.assign(reinterpret_cast<char*>(&im_data[pos]), len);
            pos += len;
        }
        if (version >= 23)
        {
            shift_pos(im_data, pos, im.splice[0]);
            shift_pos(im_data, pos, im.splice[1]);
        }
        if (version == 24) pos += sizeof(uint8_t); // @date December 2025

        items.emplace_back(im);
    }
    ALREADY_INJECTED = true;
    return "Injected " + std::to_string(items.size()) + " items from the items.dat v" + std::to_string(version);
}