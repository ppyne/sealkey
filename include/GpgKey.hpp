#pragma once

#include <string>

struct GpgKey {
    std::string type;
    std::string fingerprint;
    std::string keyId;
    std::string uid;
    std::string email;
    std::string createdAt;
    std::string expiresAt;
    std::string capabilities;
    bool hasSecretKey = false;
    bool canEncrypt = false;
    bool canSign = false;
    bool canCertify = false;
    bool canAuthenticate = false;
    bool revoked = false;
    bool expired = false;
    std::string validity;
};
