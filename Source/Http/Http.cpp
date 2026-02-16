#include "Http.hpp"
#include "../Logger/Logger.hpp"
#include "../Main/Config.hpp"
#include "../Functions/Functions.hpp"
#include "../Network/Network.hpp"

#include <thread>
#include <chrono>

HttpManager Http;

bool HttpManager::Fetcher() {
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

    while (!(res = cli.Post("/growtopia/server_data.php", headers,
        "version=" + Client.VERSION + "&protocol=" + Client.PROTOCOL + "&platform=" + Client.PLATFORM + "",
        "application/x-www-form-urlencoded"))) {

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
    Server.UDP = std::stoi(port);

    {
        enet_address_set_host(&Network.GetServerAddress(), Server.IP.c_str());
        Network.GetServerAddress().port = Server.UDP;
    }

    server_data_cache =
        "server|" + Local.IP + "\n"
        "port|" + std::to_string(Local.UDP) + "\n"
        "type|1\n"
        "loginurl|" + loginurl + "\n"
        "\n"
        "beta_type|1\n"
        "meta|" + meta + "\n"
        "RTENDMARKERBS1001";

    System::editHosts(Local.IP);
    Sleep(1000);

    LOG_INFO("Fetched the api www.growtopia.com");
    return true;
}

void HttpManager::Injector() {
    using namespace httplib;

    FastLog::Logger::set_thread_name("HTTP");

    SSLServer svr("growtopia.pem", "growtopia.key.pem");
    if (!svr.is_valid()) {
        LOG_ERROR("Invalid SSL configuration!");
        return;
    }

    svr.Get("/", [&](const Request& req, Response& res) {
        res.set_content("Simple C++ HTTP Server\nOptimized Version", "text/plain");
    });

    svr.Post("/growtopia/server_data.php", [&](const Request& req, Response& res) {
        FastLog::Logger::set_thread_name("HTTP");

        LOG_WARN("Request: /growtopia/server_data.php");

        fetching = true;
        bool ok = Fetcher();
        fetching = false;
        if (ok && server_data_cache.empty()) {
            res.status = 500;
            res.set_content("Internal Server Error: No server data available.", "text/plain");
        }
        else res.set_content(server_data_cache, "text/plain");
    });

    LOG_INFO("Started HTTP server");

    if (!svr.listen("0.0.0.0", Local.TCP)) {
        LOG_ERROR("Failed to bind to port {}!", Local.TCP);
        return;
    }
}