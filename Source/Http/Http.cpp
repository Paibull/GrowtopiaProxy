#include "Http.hpp"
#include "../Logger/Logger.hpp"
#include "../Main/Config.hpp"
#include "../Functions/Functions.hpp"
#include "../Network/Network.hpp"
#include "../Network/Handler.hpp"
#include "../Player/Player.hpp"
#include "../Items/Items.hpp"

#include <random>

#include <thread>
#include <chrono>

HttpManager Http;

bool HttpManager::Fetcher(const std::string& clientBody) {
    /* A server configured by hand wins over the fetch. Pointing the client at a
       private server is the only way to iterate on a feature without spending a
       real account on every mistake -- but read the warning in Config.hpp: a
       private server enforces nothing, so it proves a feature WORKS, never that
       it is safe on the real servers. Watch the log for
       PACKET_APP_INTEGRITY_FAIL; that fires either way. */
    if (!Server.IP.empty() && Server.UDP != 0) {
        FastLog::Logger::set_thread_name("HTTP");
        LOG_WARN("Using the server configured in Config.hpp ({}:{}); skipping the growtopia2.com fetch.",
                 Server.IP, Server.UDP);

        memset(&Network.GetServerAddress(), 0, sizeof(ENetAddress));
        Network.GetServerAddress().type = ENET_ADDRESS_TYPE_IPV4;
        if (enet_address_set_host_ip(&Network.GetServerAddress(), Server.IP.c_str()) != 0) {
            LOG_ERROR("Config.hpp Server.IP is not a valid address: {}", Server.IP);
            return false;
        }
        Network.GetServerAddress().port = Server.UDP;

        server_data_cache =
            "server|" + Local.IP + "\n"
            "port|" + std::to_string(Local.UDP) + "\n"
            "type|1\n"
            "#maint|Maintenance message\n"
            "RTENDMARKERBS1001";

        System::editHosts(Local.IP);
        return true;
    }

    if (System::IsTrueAdmin()) System::editHosts("");
    else return false;

    FastLog::Logger::set_thread_name("HTTP");

    httplib::Client cli("https://www.growtopia2.com");
    cli.enable_server_certificate_verification(false);

    httplib::Headers headers = {
        { "User-Agent", "UbiServices_SDK_2022.Release.9_PC64_ansi_stati" },
        { "Accept", "*/*" }
    };

    httplib::Result res;
    int retry_count = 0;
    const int max_retries = 3;

    /* A transport failure gives a falsy Result, but growtopia2.com answering
       503 during maintenance gives a truthy one carrying an error page. The
       old loop only retried the first case, then parsed the error page and
       reported the missing fields as if the format had changed. */
    /* Forward what the client actually sent rather than rebuilding it from
       Config. The client knows its own version and protocol; the constants in
       Config.hpp go stale on every Growtopia update and then this POST gets an
       update-required page instead of server data. They stay as the fallback
       for a request that arrives with no body. */
    const std::string body = clientBody.empty()
        ? "version=" + Client.VERSION + "&protocol=" + Client.PROTOCOL + "&platform=" + Client.PLATFORM
        : clientBody;

    LOG_DEBUG("server_data.php request body: {}", body);

    while (true) {
        res = cli.Post("/growtopia/server_data.php", headers, body,
            "application/x-www-form-urlencoded");

        if (res && res->status == 200) break;

        if (res) LOG_WARN("server_data.php returned HTTP {}", res->status);

        if (++retry_count >= max_retries) {
            LOG_ERROR("Failed to fetch server data after {} attempts", max_retries);
            return false;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::string server = getValue(res->body, "server");
    std::string port = getValue(res->body, "port");
    std::string loginurl = getValue(res->body, "loginurl");
    std::string meta = getValue(res->body, "meta");

    if (server.empty() || port.empty() || loginurl.empty() || meta.empty()) {
        server.empty() ? LOG_ERROR("server is missing") : LOG_INFO("server: {}", server);
        port.empty() ? LOG_ERROR("port is missing") : LOG_INFO("port: {}", port);
        loginurl.empty() ? LOG_ERROR("loginurl is missing") : LOG_INFO("loginurl: {}", loginurl);
        meta.empty() ? LOG_ERROR("meta is missing") : LOG_INFO("meta: {}", meta);
        return false;
    }
    LOG_DEBUG("Fetching www.growtopia2.com...");
    LOG_DEBUG("server: {}", server);
    LOG_DEBUG("port: {}", port);
    LOG_DEBUG("loginurl: {}", loginurl);
    LOG_DEBUG("meta: {}", meta);

    Server.IP = server;

    /* std::stoi throws on a non-numeric body and this runs inside an httplib
       handler, so the throw took the HTTP thread down with it. The range check
       matters too: the field is a uint16_t, and stoi would happily hand back
       something that truncates to a working-looking wrong port. */
    int parsedPort = 0;
    try { parsedPort = std::stoi(port); }
    catch (...) { parsedPort = 0; }

    if (parsedPort <= 0 || parsedPort > 65535) {
        LOG_ERROR("Bad port from server_data.php: {}", port);
        return false;
    }
    Server.UDP = static_cast<uint16_t>(parsedPort);

    {
        /* This returned a string literal from a function declared `bool`. The
           literal decays to a non-null `const char*`, which converts to `true`
           -- so an unusable server address was reported to the caller as a
           successful fetch, and the failure only surfaced later as a connection
           that never completes.

           Also `!= 0` rather than `< 0`: enet_address_set_host_ip is documented
           to return 0 on success, and does not promise a negative value on
           failure. Network.cpp already compares it that way. */
        if (enet_address_set_host_ip(&Network.GetServerAddress(), Server.IP.c_str()) != 0) {
            LOG_ERROR("Invalid server IP from server_data.php: {}", Server.IP);
            return false;
        }
        Network.GetServerAddress().port = Server.UDP;
    }

    server_data_cache =
        "server|" + Local.IP + "\n"
        "port|" + std::to_string(Local.UDP) + "\n"
        "loginurl|" + loginurl + "\n"
        "type|1\n"
        "type2|1\n"
        "#maint|Maintenance message\n"
        "\n"
        "beta_server|127.0.0.1\n"
        "beta_port|17091\n"
        "\n"
        "beta_type|1\n"
        "meta|" + meta + "\n"
        "RTENDMARKERBS1001";

    System::editHosts(Local.IP);
    Sleep(1000);

    LOG_INFO("Fetched the api www.growtopia2.com");
    return true;
}

void HttpManager::Injector() {
    using namespace httplib;

    FastLog::Logger::set_thread_name("HTTP");

    ensure_cert_files_exist();

    /* ensure_cert_files_exist is synchronous, so if the files still are not
       there the write failed -- a read-only or non-writable working directory.
       The old code span here forever waiting for a second writer that does not
       exist, with no output at all. */
    if (!std::filesystem::exists(CERT_FILE) || !std::filesystem::exists(KEY_FILE)) {
        LOG_ERROR("Couldn't write {} / {}. Is the working directory writable?", CERT_FILE, KEY_FILE);
        return;
    }

    SSLServer svr(CERT_FILE, KEY_FILE);
    if (!svr.is_valid()) {
        LOG_ERROR("Invalid SSL configuration!");
        return;
    }

    {
        std::random_device rd;
        std::uniform_int_distribution<int> hex(0, 15);
        control_token.clear();
        for (int i = 0; i < 32; ++i) control_token += "0123456789abcdef"[hex(rd)];
        LOG_WARN("Control token: {}", control_token);
    }

    auto authorised = [this](const Request& req) {
        auto it = req.headers.find("X-Proxy-Token");
        return it != req.headers.end() && it->second == control_token;
    };

    svr.Post("/proxy/command", [&](const Request& req, Response& res) {
        FastLog::Logger::set_thread_name("HTTP");
        if (!authorised(req)) { res.status = 403; res.set_content("bad token", "text/plain"); return; }

        std::string cmd = req.body;
        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) cmd.pop_back();
        if (cmd.empty() || cmd[0] != '/') { res.status = 400; res.set_content("expected a /command", "text/plain"); return; }

        if (!RunProxyCommand(cmd)) { res.status = 409; res.set_content("no live session", "text/plain"); return; }
        res.set_content("ran " + cmd, "text/plain");
    });

    svr.Get("/proxy/status", [&](const Request& req, Response& res) {
        FastLog::Logger::set_thread_name("HTTP");
        if (!authorised(req)) { res.status = 403; res.set_content("bad token", "text/plain"); return; }

        res.set_content(
            "session|"  + std::string(Network.CurrentServerPeer() ? "live" : "none") + "\n"
            "name|"     + Player.tankIDName + "\n"
            "netID|"    + std::to_string(Player.netID) + "\n"
            "world|"    + Player.world + "\n"
            "items|"    + std::to_string(items.size()) + "\n",
            "text/plain");
    });

    svr.Get("/", [&](const Request& req, Response& res) {
        res.set_content("Simple C++ HTTP Server\nOptimized Version", "text/plain");
    });

    svr.Post("/growtopia/server_data.php", [&](const Request& req, Response& res) {
        FastLog::Logger::set_thread_name("HTTP");

        LOG_WARN("Request: /growtopia/server_data.php");

        /* If a fetch is already in flight, this request is almost certainly
           our own outbound POST arriving back here -- the redirect to
           127.0.0.1 outliving the hosts edit. Answering it instead of
           recursing keeps one bad resolution from turning into a stack of
           nested fetches. */
        bool inFlight = false;
        if (!fetching.compare_exchange_strong(inFlight, true)) {
            LOG_ERROR("server_data.php hit while a fetch is in flight -- the proxy resolved "
                      "www.growtopia2.com to itself. DNS cache still holds the redirect.");
            res.status = 508;
            res.set_content("Loop detected: the proxy resolved growtopia2.com to itself.", "text/plain");
            return;
        }

        bool ok = Fetcher(req.body);
        fetching = false;

        if (!ok) {
            /* Fetcher clears the hosts redirect on entry so it can reach the
               real growtopia2.com, and only puts it back on the success path.
               Every early return therefore left the client resolving the real
               server and bypassing the proxy entirely. */
            System::editHosts(Local.IP);
        }

        /* This was `ok && cache.empty()`, so a failed fetch fell to the else
           and answered 200 with a stale or empty body -- the client then sat
           on a connection that never completed instead of showing an error. */
        if (!ok || server_data_cache.empty()) {
            res.status = 500;
            res.set_content("Internal Server Error: No server data available.", "text/plain");
        }
        else res.set_content(server_data_cache, "text/plain");
    });

    LOG_INFO("Started HTTP server");

    if (!svr.listen(Local.IP, Local.TCP)) {
        LOG_ERROR("Failed to bind to port {}!", Local.TCP);
        return;
    }
}