#pragma once
#include "ENET/enet.h"
#include <string>
#include <mutex>
#include <functional>
#include <sol/sol.hpp>

class LuaManager {
public:
    static LuaManager& Get() {
        static LuaManager instance;
        return instance;
    }

    void Inject(const std::string& filename);

    sol::state& State() { return m_lua; }

    /* Kept only so /lua can refuse to start with no session. The bindings
       resolve peers through NetworkManager at call time instead: a script
       outlives world changes, and each one replaces both peers. */
    void SetPeers(ENetPeer* client, ENetPeer* server) {
        m_client = client;
        m_server = server;
    }

    void StopScript() { m_running = false; }

private:
    LuaManager();
    ~LuaManager() = default;

    LuaManager(const LuaManager&) = delete;
    LuaManager& operator=(const LuaManager&) = delete;

    /* Marks a script as occupying the interpreter, from Inject until the
       script thread exits. Not a mutex: it is claimed on the caller's
       thread and released on the script thread, and std::mutex may only be
       unlocked by the thread that locked it. */
    std::atomic<bool> m_scriptActive = false;
    sol::state m_lua;
    std::atomic<bool> m_running = false;

    ENetPeer* m_client = nullptr;
    ENetPeer* m_server = nullptr;

    void RegisterFunctions();
};