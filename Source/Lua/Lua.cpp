#include "Lua.hpp"
#include "../Logger/Logger.hpp"
#include "../Player/Player.hpp"
#include "../Packets/Packets.hpp"
#include "../Network/Handler.hpp"
#include <filesystem>

/* @important: Put in all for-while codes this line: 
        if not isRunning() then return end
*/

LuaManager::LuaManager() {
    m_lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::io,
        sol::lib::os
    );

    RegisterFunctions();
}

void LuaManager::RegisterFunctions() {

    /* @note: Main Functions */
    m_lua.set_function("print", [](sol::variadic_args args) {
        for (auto v : args) LOG_WARN("Print: {}", v.as<std::string>());
    });

    m_lua.set_function("sleep", [this](int ms) {
        int elapsed = 0;
        while (elapsed < ms && m_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            elapsed += 100;
        }
    });

    m_lua.set_function("isRunning", [this]() { return m_running.load(); });
    /* @note: Main Functions */



    /* @note: Get From Player Functions */
    m_lua.set_function("getUserID", []() { return Player.userID; });
    m_lua.set_function("getTankIDName", []() { return Player.tankIDName; });
    m_lua.set_function("getPlayerAge", []() { return Player.player_age; });
    m_lua.set_function("getNetID", []() { return Player.netID; });
    m_lua.set_function("getCountry", []() { return Player.country; });

    m_lua.set_function("getWorld", []() { return Player.world; });
    m_lua.set_function("getLastWorld", []() { return Player.lastWorld; });
    m_lua.set_function("getSaveWorld", []() { return Player.saveWorld; });

    m_lua.set_function("getKlv", []() { return Player.klv; });
    m_lua.set_function("getMeta", []() { return Player.meta; });
    m_lua.set_function("getRid", []() { return Player.rid; });
    m_lua.set_function("getMac", []() { return Player.mac; });
    m_lua.set_function("getWk", []() { return Player.wk; });

    m_lua.set_function("getPosition", []() -> std::pair<float, float> { return { Player.position.x, Player.position.y }; });

    m_lua.set_function("getItemCount", [](int id) { return Player.GetItemCount(id); });
    m_lua.set_function("hasItem", [](int id) { return Player.HasItem(id); });
    /* @note: Get From Player Functions */



    /* @note: Some Player Functions */
    m_lua.set_function("warp", [this](const std::string& worldName) {
        if (!m_client || !m_server) { LOG_ERROR("Peers not set!"); return; }
        Player.Warp(worldName, m_client, m_server);
    });

    m_lua.set_function("drop", [this](int id, int count) {
        if (!m_client || !m_server) { LOG_ERROR("Peers not set!"); return; }
        Player.Drop(id, count, m_client, m_server);
    });

    m_lua.set_function("trash", [this](int id, int count) {
        if (!m_client || !m_server) { LOG_ERROR("Peers not set!"); return; }
        Player.Trash(id, count, m_client, m_server);
    });
    /* @note: Some Player Functions */



    /* @note: Some Variant Packets */
    m_lua.set_function("OnConsoleMessage", [this](const std::string& text) {
        if (!m_client) { LOG_ERROR("Peers not set!"); return; }
        GamePacket<OnConsoleMessage> p;
        p.text = text;
        SendGamePacket(m_client, p);
    });

    m_lua.set_function("OnTalkBubble", [this](uint32_t netId, const std::string& text) {
        if (!m_client) { LOG_ERROR("Peers not set!"); return; }
        GamePacket<OnTalkBubble> p;
        p.netId = netId;
        p.text = text;
        SendGamePacket(m_client, p);
    });

    m_lua.set_function("OnTextOverlay", [this](const std::string& text) {
        if (!m_client) { LOG_ERROR("Peers not set!"); return; }
        GamePacket<OnTextOverlay> p;
        p.text = text;
        SendGamePacket(m_client, p);
    });

    m_lua.set_function("OnDialogRequest", [this](const std::string& text) {
        if (!m_client) { LOG_ERROR("Peers not set!"); return; }
        GamePacket<OnDialogRequest> p;
        p.text = text;
        SendGamePacket(m_client, p);
    });

    m_lua.set_function("OnSetFreezeState", [this](uint32_t seconds) {
        if (!m_client) { LOG_ERROR("Peers not set!"); return; }
        GamePacket<OnSetFreezeState> p;
        p.seconds = seconds;
        SendGamePacket(m_client, p);
    });
    /* @note: Some Variant Packets */



    /* @note: Some Custom Packets */
    m_lua.set_function("sendPacket", [this](const std::string& target, int type, const std::string& text) {
        if (!m_client || !m_server) { LOG_ERROR("Peers not set!"); return; }
        ENetPeer* peer = (target == "server") ? m_server : m_client;
        SendPacket(peer, type, text.c_str(), text.length());
    });

    m_lua.set_function("sendPlayerMoving", [this](sol::table t) {
        if (!m_client || !m_server) { LOG_ERROR("Peers not set!"); return; }

        PlayerMovingPacket pkt;

        if (t["packetType"].valid())     pkt.PacketType(t["packetType"].get<int>());
        if (t["netID"].valid())          pkt.NetID(t["netID"].get<int>());
        if (t["characterState"].valid()) pkt.CharacterState(t["characterState"].get<int>());
        if (t["plantingTree"].valid())   pkt.PlantingTree(t["plantingTree"].get<int>());
        if (t["punch"].valid()) {
            sol::table punch = t["punch"];
            pkt.Punch(punch[1].get<int>(), punch[2].get<int>());
        }
        if (t["position"].valid()) {
            sol::table pos = t["position"];
            pkt.Position(pos[1].get<float>(), pos[2].get<float>());
        }
        if (t["speed"].valid()) {
            sol::table spd = t["speed"];
            pkt.Speed(spd[1].get<float>(), spd[2].get<float>());
        }

        std::string target = t["target"].valid() ? t["target"].get<std::string>() : "server";
        ENetPeer* peer = (target == "client") ? m_client : m_server;

        pkt.Send(peer);
    });
    /* @note: Some Custom Packets */
}

void LuaManager::Inject(const std::string& filename) {
    if (!m_mutex.try_lock()) {
        LOG_WARN("A script is already running, ignoring {}", filename);
        return;
    }

    m_running = true;

    std::thread([this, filename]() {
        if (!std::filesystem::exists(filename)) {
            LOG_ERROR("Couldn't find the {}", filename);
            m_running = false;
            m_mutex.unlock();
            return;
        }

        auto result = m_lua.safe_script_file(filename, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            LOG_ERROR("Error in ({}): {}", filename, err.what());
        }
        else LOG_DEBUG("{} injected successfully", filename);

        m_running = false;
        m_mutex.unlock();
    }).detach();
}