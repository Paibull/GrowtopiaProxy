#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "HTTP/Http_Extension.h"
#include <fstream>
#include <filesystem>
#include <atomic>

class HttpManager {
public:
    static constexpr const char* CERT_FILE = "proxy.pem";
    static constexpr const char* KEY_FILE = "proxy.key.pem";

    bool Fetcher(const std::string& clientBody = "");
    void Injector();

public:
    std::atomic<bool> fetching = false;
    std::string server_data_cache;

    std::string control_token;

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
        const char* cert_file = CERT_FILE;
        const char* key_file = KEY_FILE;

        if (!std::filesystem::exists(cert_file)) {
            std::ofstream(cert_file) << R"(-----BEGIN CERTIFICATE-----
MIIEBDCCAuygAwIBAgIUPEv6ZOUlX8kZ9Y88WYZIxeiT1r8wDQYJKoZIhvcNAQEL
BQAwYTELMAkGA1UEBhMCSUQxCzAJBgNVBAgMAkpCMRMwEQYDVQQHDApKYXdhIEJh
cmF0MRcwFQYDVQQKDA5Hcm93dG9waWFQcm94eTEXMBUGA1UEAwwOZ3Jvd3RvcGlh
MS5jb20wHhcNMjYwOTA0MTg0MzE0WhcNNDYwODMwMTg0MzE0WjBhMQswCQYDVQQG
EwJJRDELMAkGA1UECAwCSkIxEzARBgNVBAcMCkphd2EgQmFyYXQxFzAVBgNVBAoM
Dkdyb3d0b3BpYVByb3h5MRcwFQYDVQQDDA5ncm93dG9waWExLmNvbTCCASIwDQYJ
KoZIhvcNAQEBBQADggEPADCCAQoCggEBANPPQ4n9oAvcrBe5pwSACv9WjCYZN4Y4
WfAAlLmiEEN6ny/pmmYlAeIkXA+sWxAlNvqvKZcwO37VkYOm0phKZRGzZcbrG2KM
rzfd/2bFfqe6cnty1PMOBPvxaV1rR1AR0wcxaZ0JFYNN0G0lJwukgXW81d+T4Kt+
gUECRDukvGblDkBeV4iwEL+UqJ0MBYoUWK13a25EYAvNErRPxbNKa8THWiJ1FKmg
rJ4OmStzD7ZVjRzlK3VNquM4E/jczKSXlSAKAe7MkdveYhZJOH6FC4KjAb7Zeu9L
PDyd+IxWIU66S07n9WB0SbF+zThi6UH4neCer12ZVZSQdR2O6V54RKECAwEAAaOB
szCBsDAdBgNVHQ4EFgQURtLaMJ9rl28SqH++wPkiD8MnzGcwCQYDVR0TBAIwADAL
BgNVHQ8EBAMCBaAwEwYDVR0lBAwwCgYIKwYBBQUHAwEwYgYDVR0RBFswWYIOZ3Jv
d3RvcGlhMS5jb22CEnd3dy5ncm93dG9waWExLmNvbYIOZ3Jvd3RvcGlhMi5jb22C
End3dy5ncm93dG9waWEyLmNvbYIJbG9jYWxob3N0hwR/AAABMA0GCSqGSIb3DQEB
CwUAA4IBAQBdJAmch7sPOFKLU5zLpQLrY76tgpJDcMF0kOSfMgs6QkKvIz3Zw90g
nAXZpgZQE3xpnLmvFbqsT8C7ABaOloWGFqNjQApujG1hhK6hzrh0Z0LP4vu0gT/p
HLxkdD3Bn85NLwhcTS95St1KICtg6TMKGo3f6vf1cjQ2jjv/Z2sUDaz/0zmI6RKx
wa7bAW3+3reGkKIakTzK0npVcW+0USmO77t5mRYJXiZL6MMK/nHtRQA0DXwQiZ5D
jrBT5GJP4kMci9FLKkCi/DLdSPxEAgGIhXiQILajmTPW5tafeLjwYgb0yAIVxdsl
oOzO4iWw/7TsNVYcMD4ezt0fTUvip9Ek
-----END CERTIFICATE-----)";
        }

        if (!std::filesystem::exists(key_file)) {
            std::ofstream(key_file) << R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDTz0OJ/aAL3KwX
uacEgAr/VowmGTeGOFnwAJS5ohBDep8v6ZpmJQHiJFwPrFsQJTb6rymXMDt+1ZGD
ptKYSmURs2XG6xtijK833f9mxX6nunJ7ctTzDgT78Wlda0dQEdMHMWmdCRWDTdBt
JScLpIF1vNXfk+CrfoFBAkQ7pLxm5Q5AXleIsBC/lKidDAWKFFitd2tuRGALzRK0
T8WzSmvEx1oidRSpoKyeDpkrcw+2VY0c5St1TarjOBP43Mykl5UgCgHuzJHb3mIW
STh+hQuCowG+2XrvSzw8nfiMViFOuktO5/VgdEmxfs04YulB+J3gnq9dmVWUkHUd
juleeEShAgMBAAECggEABojAYztySQcIJs5PcE9rdfpyq798leID+P9paJtdbyse
7OLIx3QA57vtJSG563LoRlbhSaEMbNHYe9xJfkKX0mx4g4uuZ7qq4PIEVnAT7FL+
A/R4nfGluuWjhs3YkfDrjgDApOz13IeePR8DVpPtkZQ5h/EMVpyXPWFl7b948ba0
7vmocNHlPoY8B5QjccGIgru4WHpWCzV1udjVZm6AVQsaxLAV+qxXW5SyMvURPf5q
kBvelgauEzSI2SAlmunD/Uec3126ZDB2cWWKEWeNYmoput665pu3NvT1tJehxrcy
dWvF0KyzG3mENtOENgvTfvHqCWEDZvB+WAVUiIZI2wKBgQD+IpBKYCERwhzMnw+F
e7S+lh2WQNajzI72SiolRKv3rsHoPsteT1S8eK44asurtABxU6Q8ZFHdHYwuUAHM
j9CpoNQMqSLVYRkgylh8EYZd66OixFue145O3RBIEZ0HitaC9a0uIdjP+9IgqkfD
se72RP8aLFijFeJjlDfxVAUYzwKBgQDVXS9F2mhkMaOTxb69krOZTDiBBhzYLb1W
4XUM6E2hNxbJf1butmnc5duBRl31nbwOYlSU4QSV3yv9tvUGv/3SnCvQmcABnSe+
f0O/J72apB031jPNuupX7qX1Bt+XzT14zzwPh3QAvDhan0HQOSoz4EIEfXpz8DKo
PjspseJHjwKBgDdynR7NKInyW0w97CoImp/2qs/sp6gnao3MErP87rRkucQNZ1vV
XTyd7A09J+D3rh0LzcqrbL0cxEgahrn2KuXHxFxaztHVlKD15SZ0wGdfkV1jEEZw
64jDbNj8ltFddn4uUjG9isueOvOLk4rcGLI8zZgNUu2KSdHGNgp+dXo7AoGBAKtk
eIXvZW4e5dzdu1QDVVwuezFB3MfXLkJtR55/uWRooVhpf0awp6d3yXU2NmIIPDl9
yZ3yh8FwjaD4aCns7hNRumyOJUvmlzeSebRDUy626HWjDugTXw4VuaBzzgbeKqQy
LHf5AjlY+Rfq2G5QjVMwsTd0KHqbl8XIf7QFndSHAoGBAKBNJtD1cPIxn6Vxzkkc
kbJXOov2R2mekKGwwpRWKIaS4ccC1Jg0PtRm7MtTuvAfzI1Xq9aoNk45zWYjQXyr
gD/arJvy6bcWGpGTSq0ghSKNIKefsodkcXFAFqA2lOCfAkPJn8DdfImbQ1HOjOB2
Frr1SE1MIj0rmGruN1mXl7Zx
-----END PRIVATE KEY-----)";
        }
    }

};
extern HttpManager Http;