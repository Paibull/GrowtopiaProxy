#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "HTTP/Http_Extension.h"

class HttpManager {
public:
    bool Fetcher();
    void Injector();

public:
    bool fetching = false;
    std::string server_data_cache;

private:
    static auto getValue(const std::string& src, const std::string& key) -> std::string {
        std::string search = key + "|";
        size_t pos = src.find(search);
        if (pos == std::string::npos) return "";

        pos += search.length();
        size_t end = src.find("\n", pos);
        if (end == std::string::npos) end = src.length();

        return src.substr(pos, end - pos);
    }

};
extern HttpManager Http;