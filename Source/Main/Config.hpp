#pragma once

#include <unordered_map>
#include <algorithm>
#include <string>
#include "../Network/Network.hpp"


struct gMain {
    std::string NAME = "Growtopia Proxy";
    std::string VERSION = "1.0";
    bool BASIC_LOGS = false;
};
inline gMain Main;


struct gLocal {
	std::string IP = "127.0.0.1";
	uint16_t TCP = 443;
	uint16_t UDP = 17123;
};
inline gLocal Local;


struct gServer {
	/* Leave both empty to use the real Growtopia servers: they are filled in
	   from www.growtopia2.com on the first 'Play Online'.

	   Set them to point the client at a private server instead. That is the
	   only way to iterate on a feature without spending a real account on
	   every mistake -- but be clear about what it buys you. A private server
	   enforces nothing, so it can show a feature WORKS; it can never show a
	   feature is SAFE.

	   The ban does not come from the server. It comes from the client, which
	   reports its own state with PACKET_APP_INTEGRITY_FAIL to whatever server
	   it happens to be talking to. A private server receives that and shrugs;
	   the real one bans for it. So watch the log for that packet -- it is
	   raised loudly, and on a private server it is the only thing that will
	   ever tell you a feature would have cost you an account. */
	std::string IP = ""; // empty = fetch from growtopia2.com
	/* 0, not -1: the field is unsigned, so -1 silently became port 65535 -- a
	   real port number that would be used as-is if the fetch never ran. */
	uint16_t UDP = 0; // 0 = fetch from growtopia2.com
};
inline gServer Server;


struct gClient {
    /* @info: Fallback only. The proxy forwards the client's own POST body to
       growtopia2.com, so these are used solely when a request arrives without
       one -- you no longer have to bump them on every Growtopia update. */
    std::string VERSION = "5.44";
    std::string PROTOCOL = "225";
    std::string PLATFORM = "0,1,1"; /* @info: Don't change if you are on 'Windows' */
};
inline gClient Client;


inline bool ShouldLogNetMessage(int type, int extra = 0) {
    /* @important: Customize the Logging System
        return true = print
        return false = !print
    */

    switch (type) {
        case Network.NET_MESSAGE_SERVER_HELLO: return true;

        case Network.NET_MESSAGE_GENERIC_TEXT: return true;
        case Network.NET_MESSAGE_GAME_MESSAGE: return true;

        case Network.NET_MESSAGE_GAME_PACKET: {
            switch (extra) {
                case Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC: return true;
                case Network.LOG_NET_MESSAGE_GAME_PACKET_BASIC_CLEAR_SPAM: return true;
                case Network.LOG_NET_MESSAGE_GAME_PACKET_PLAYER_MOVING: return true;
                default: return true;
            }
        }

        case Network.NET_MESSAGE_ERROR: return true;
        case Network.NET_MESSAGE_TRACK: return true;

        case Network.NET_MESSAGE_CLIENT_LOG_REQUEST: return true;
        case Network.NET_MESSAGE_CLIENT_LOG_RESPONSE: return true;

        case Network.NET_MESSAGE_UNKNOWN: 
        default:
        { return true; }
    }
}