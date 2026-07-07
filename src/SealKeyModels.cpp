#include "SealKeyModels.hpp"

#include <algorithm>

namespace {
std::string trim(std::string value) {
    auto notSpace = [](unsigned char c) { return c != ' ' && c != '\t' && c != '\r' && c != '\n'; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}
}

std::string displayNameFromUid(const std::string& uid) {
    auto marker = uid.find('<');
    if (marker == std::string::npos) {
        return uid;
    }
    return trim(uid.substr(0, marker));
}

std::string shortFingerprint(const std::string& fingerprint) {
    if (fingerprint.size() <= 8) {
        return fingerprint;
    }
    return fingerprint.substr(fingerprint.size() - 8);
}

GpgIdentity identityFromKey(const GpgKey& key) {
    GpgIdentity identity;
    identity.name = displayNameFromUid(key.uid);
    identity.email = key.email;
    identity.fingerprint = key.fingerprint;
    identity.keyId = key.keyId;
    identity.canSign = key.hasSecretKey && key.canSign;
    identity.canDecrypt = key.hasSecretKey && key.canEncrypt;
    identity.createdAt = key.createdAt;
    identity.expiresAt = key.expiresAt;
    return identity;
}

GpgContact contactFromKey(const GpgKey& key) {
    GpgContact contact;
    contact.name = displayNameFromUid(key.uid);
    contact.email = key.email;
    contact.fingerprint = key.fingerprint;
    contact.keyId = key.keyId;
    contact.canEncrypt = key.canEncrypt;
    contact.canVerify = key.canSign || key.canCertify;
    contact.trustLevel = key.validity;
    contact.createdAt = key.createdAt;
    contact.expiresAt = key.expiresAt;
    return contact;
}
