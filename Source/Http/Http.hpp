#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "HTTP/Http_Extension.h"
#include <fstream>
#include <filesystem>

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

    void ensure_cert_files_exist() {
        const char* cert_file = "growtopia.pem";
        const char* key_file = "growtopia.key.pem";

        if (!std::filesystem::exists(cert_file)) {
            std::ofstream(cert_file) << R"(-----BEGIN CERTIFICATE-----
MIIEZTCCA02gAwIBAgIUFS6HfcSIh/GshrTCiifox7GRITcwDQYJKoZIhvcNAQEL
BQAwgYIxCzAJBgNVBAYTAklEMQswCQYDVQQIDAJKQjETMBEGA1UEBwwKSmF3YSBC
YXJhdDEXMBUGA1UECgwOVGVhbU5ldm9sdXRpb24xFzAVBgNVBAMMDmdyb3d0b3Bp
YTEuY29tMR8wHQYJKoZIhvcNAQkBFhBuaXhpYzBAcHJvdG9uLm1lMB4XDTI0MDQx
MjAwNTcwNVoXDTI1MDQxMjAwNTcwNVowgYIxCzAJBgNVBAYTAklEMQswCQYDVQQI
DAJKQjETMBEGA1UEBwwKSmF3YSBCYXJhdDEXMBUGA1UECgwOVGVhbU5ldm9sdXRp
b24xFzAVBgNVBAMMDmdyb3d0b3BpYTEuY29tMR8wHQYJKoZIhvcNAQkBFhBuaXhp
YzBAcHJvdG9uLm1lMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArLW7
mxy29be21CUcnybB29rlw5TR1B6ToDWUu7YKSKIi2dTK87mVMPXOZU9LMGtpYU9o
GeSU3tkwgWsWwGs3eenzsrCxaNCDy41Tj0GZVNHbUfFowxlDcy7xKma+TjasyB7O
e49e/Qi6hnSgJbZYOlF3+cux64junWDCv4QmrizLOjp5sIpbiRj6wVzAVf538ruP
HNmsv28QwSElWxkqUG2ONcW143E9/052ez5jNx8TFJTqXBsT0ZYmEfDNfI3dGOMx
WzAu8OW83pQKAO98qPyXqf4uEncYsTokpcrhifsBECFxM3gi3ruIh7Zb7/h6IgU2
0tjEZs8E6qnDB4CjOwIDAQABo4HQMIHNMB0GA1UdDgQWBBQEVBwF/WyNCLc9OADz
lWa3e8EFRzAJBgNVHRMEAjAAMAsGA1UdDwQEAwIF4DATBgNVHSUEDDAKBggrBgEF
BQcDATBRBgNVHREESjBIgg5ncm93dG9waWExLmNvbYISd3d3Lmdyb3d0b3BpYTEu
Y29tgg5ncm93dG9waWEyLmNvbYISd3d3Lmdyb3d0b3BpYTIuY29tMCwGCWCGSAGG
+EIBDQQfFh1PcGVuU1NMIEdlbmVyYXRlZCBDZXJ0aWZpY2F0ZTANBgkqhkiG9w0B
AQsFAAOCAQEAlenKUyG6iTpSgC+P+MJ/46Bfqf3FTRjW9LK3LziaLZXc29h6BDCV
PiKQ6qh/2nDFi4KUWIV50/XnSWhqVE2wlLcahvYeybLxvALL77NfZSwKRFnCJCpl
9ZwTId8PNgbUldwjogSv8TnphQ5RsjBVI5CSToHSMUw/zwcsBUc7DIbCXkecnoV2
QcwEQXcjEPwXeQQb2qJX4QybTkmCpaVtf5ZsZVwJMMRjYlc7AymGcsONZkn4ZX31
PWL48vTB0rqVX/5IlP9eyAJ0uotP08USAsvNYQBaUnaqp9Xku/HiZZMJxrs5Gkpy
BNbTRAlg/RXjV+kED1yYLrJcSiTS60oaew==
-----END CERTIFICATE-----)";
        }

        if (!std::filesystem::exists(key_file)) {
            std::ofstream(key_file) << R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCstbubHLb1t7bU
JRyfJsHb2uXDlNHUHpOgNZS7tgpIoiLZ1MrzuZUw9c5lT0swa2lhT2gZ5JTe2TCB
axbAazd56fOysLFo0IPLjVOPQZlU0dtR8WjDGUNzLvEqZr5ONqzIHs57j179CLqG
dKAltlg6UXf5y7HriO6dYMK/hCauLMs6OnmwiluJGPrBXMBV/nfyu48c2ay/bxDB
ISVbGSpQbY41xbXjcT3/TnZ7PmM3HxMUlOpcGxPRliYR8M18jd0Y4zFbMC7w5bze
lAoA73yo/Jep/i4SdxixOiSlyuGJ+wEQIXEzeCLeu4iHtlvv+HoiBTbS2MRmzwTq
qcMHgKM7AgMBAAECggEAT+dabxZ3XTJMT1UGK2mieeWJtJVXCKTG1vHDKJ89ncIJ
jq9A4EbFi9vFTCDF4BZpsEAtIQGckId6Rf32DjrsdSZ0fYD660vBFf0CIj6Owmu+
5Ofj+JNpHdKjd+MF+6iXApUiIY9Aup99sHQnnZsBsaV8dOC1JYv6HWylXTa2MJE8
W6VANl5GjTwW4JnnutzueIoWb0xcgwTL7gmY6/0xphsgrTcowT8Wp2xJmuC30Pop
xVGvurqo2VSy1t8p5ZspVhi/F1cLw+T713MshxYLNqjSgAFjIakYlhmqfSEE9pwP
WpZ6w+JARsUXrUTOCWvrAWia9COREDD14kxEEfqnxQKBgQDnHJOPiwV/p+5qm52n
TUO4M4pkysqdyIjTmcrOdnaUV6wpCrfQ3iilZFOkoV4bOdWEzhkSHyk2/pWQVk53
ANg8WOWGLGxmOBes9Wrpw9xhpTHtSLnQNT13JMnfkQovuCLEIv0rvvRhUxid4Z2K
1PyeJpW6CcJs95tJJ4KzLjyH7QKBgQC/Txo+q3TrQAyK5f+ZvBb92+cFrJETa1Gx
764OM4ObV91w3hI40uEgWYmcGLLfVcrqRZfnr3w5SPk6v8iuF1QeYbBYSp8qUh8l
44mScC0UCxT+SgylIdprg93cZ6ONZ0ROYZp7Cma4ABS566J9/2vuCg3I4I4GhHdG
De6hDkuixwKBgQDdKBFk2UonwJF73vJceKmHCXszvINrvcyBgLf8HoyWo9cRZzpD
W3RV4M3RysF9GDjk3zxKhRsxjymcd5qacmp6RS9O2A/bOW4eirMGg/DOWACQ6nk1
bt0kuEWd9PNkerZ6LmlKhW7h+1yhKJdTlUEwLgg1gMVW9RLaTD7cLDCseQKBgQCc
VpN3BXztxxC47c5cnwYW/P8ldumz+e7wP5N5DYYOi9ZuJzyy7TqGykUXqGN8+cpy
et7xukoFMmpiJVplUrEuhtyquoip+CR2PSUu47ci+w9z46XDyQ+K719+f797jhmX
CFJLHKvm0EIf0b2fw/06sUKkl0XZc6VZPYd2XI1Y/wKBgDFMGIBCiZXzbRgL6KJi
xgYQb8x4OJPZmhVAHji8T0EarYqE5nbWDpn/O0Caiscpoo0aQIVFlDdDvEeW6NOF
Ry7P+HCbIaIhBf87GnIvWUQxSKgVNNOadLf2x90OmoT4qz6H30UKRu9i4lveV77x
yauqmi59WXT60q8HOF1smufq
-----END PRIVATE KEY-----)";
        }
    }

};
extern HttpManager Http;