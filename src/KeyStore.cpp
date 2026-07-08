#include "KeyStore.hpp"

#include "GpgProcess.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>

namespace {
std::vector<std::string> splitColonLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    std::istringstream input(line);
    while (std::getline(input, field, ':')) {
        fields.push_back(field);
    }
    if (!line.empty() && line.back() == ':') {
        fields.emplace_back();
    }
    return fields;
}

std::string fieldAt(const std::vector<std::string>& fields, std::size_t index) {
    return index < fields.size() ? fields[index] : std::string{};
}

bool hasCapability(const std::string& capabilities, char wanted) {
    auto upperWanted = static_cast<char>(std::toupper(static_cast<unsigned char>(wanted)));
    return std::any_of(capabilities.begin(), capabilities.end(), [upperWanted](unsigned char c) {
        return static_cast<char>(std::toupper(c)) == upperWanted;
    });
}

std::string extractEmail(const std::string& uid) {
    auto start = uid.find('<');
    auto end = uid.find('>', start == std::string::npos ? 0 : start);
    if (start == std::string::npos || end == std::string::npos || end <= start + 1) {
        return {};
    }
    return uid.substr(start + 1, end - start - 1);
}

void applyCapabilities(GpgKey& key) {
    key.canEncrypt = hasCapability(key.capabilities, 'e');
    key.canSign = hasCapability(key.capabilities, 's');
    key.canCertify = hasCapability(key.capabilities, 'c');
    key.canAuthenticate = hasCapability(key.capabilities, 'a');
}

bool isPrimaryRecord(const std::string& recordType) {
    return recordType == "pub" || recordType == "sec";
}

std::string upperAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

bool hasProfileId(const std::string& profileId, const char* expected) {
    return profileId == expected;
}
}

KeyStore::KeyStore(std::string gpgExecutable) : gpgExecutable_(std::move(gpgExecutable)) {}

KeyStoreResult KeyStore::listPublicKeys() const {
    auto process = GpgProcess::run(gpgExecutable_, {"--list-keys", "--with-colons", "--fingerprint", "--fingerprint"});
    KeyStoreResult result;
    result.exitCode = process.exitCode;
    if (process.success()) {
        result.keys = parseColonListing(process.standardOutput, false);
    } else {
        result.errorText = process.errorMessage.empty() ? process.standardError : process.errorMessage;
    }
    return result;
}

KeyStoreResult KeyStore::listSecretKeys() const {
    auto process = GpgProcess::run(gpgExecutable_, {"--list-secret-keys", "--with-colons", "--fingerprint", "--fingerprint"});
    KeyStoreResult result;
    result.exitCode = process.exitCode;
    if (process.success()) {
        result.keys = parseColonListing(process.standardOutput, true);
    } else {
        result.errorText = process.errorMessage.empty() ? process.standardError : process.errorMessage;
    }
    return result;
}

std::vector<GpgKey> KeyStore::listMergedKeys(std::string* errorText) const {
    auto publicResult = listPublicKeys();
    auto secretResult = listSecretKeys();
    if (errorText) {
        errorText->clear();
        if (!publicResult.success()) {
            *errorText += publicResult.errorText;
        }
        if (!secretResult.success()) {
            if (!errorText->empty()) {
                *errorText += "\n";
            }
            *errorText += secretResult.errorText;
        }
    }

    std::map<std::string, GpgKey> merged;
    for (const auto& key : publicResult.keys) {
        if (!key.fingerprint.empty()) {
            merged[key.fingerprint] = key;
        }
    }
    for (const auto& secret : secretResult.keys) {
        if (secret.fingerprint.empty()) {
            continue;
        }
        auto& key = merged[secret.fingerprint];
        if (key.fingerprint.empty()) {
            key = secret;
        }
        key.hasSecretKey = true;
        key.canSign = key.canSign || secret.canSign;
        key.canCertify = key.canCertify || secret.canCertify;
        key.canAuthenticate = key.canAuthenticate || secret.canAuthenticate;
        if (key.uid.empty()) {
            key.uid = secret.uid;
            key.email = secret.email;
        }
    }

    std::vector<GpgKey> keys;
    for (auto& [fingerprint, key] : merged) {
        (void)fingerprint;
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end(), [](const GpgKey& left, const GpgKey& right) {
        return left.uid < right.uid;
    });
    return keys;
}

std::string KeyStore::exportPublicKey(const std::string& fingerprint, std::string* errorText) const {
    if (errorText) {
        errorText->clear();
    }
    auto result = GpgProcess::run(gpgExecutable_, {"--armor", "--export", fingerprint});
    if (!result.success()) {
        if (errorText) {
            *errorText = result.errorMessage.empty() ? result.standardError : result.errorMessage;
        }
        return {};
    }
    return result.standardOutput;
}

std::string KeyStore::exportSecretKey(const std::string& fingerprint, std::string* errorText) const {
    if (errorText) {
        errorText->clear();
    }
    auto result = GpgProcess::run(gpgExecutable_, {"--armor", "--export-secret-keys", fingerprint});
    if (!result.success()) {
        if (errorText) {
            *errorText = result.errorMessage.empty() ? result.standardError : result.errorMessage;
        }
        return {};
    }
    return result.standardOutput;
}

GpgProcessResult KeyStore::importPublicKey(const std::string& filePath) const {
    return GpgProcess::run(gpgExecutable_, {"--import", filePath});
}

GpgProcessResult KeyStore::importPrivateKey(const std::string& filePath) const {
    return GpgProcess::run(gpgExecutable_, {"--import", filePath});
}

GpgProcessResult KeyStore::deletePublicKey(const std::string& fingerprint) const {
    return GpgProcess::run(gpgExecutable_, {"--batch", "--yes", "--delete-key", fingerprint});
}

GpgProcessResult KeyStore::deleteSecretAndPublicKey(const std::string& fingerprint) const {
    return GpgProcess::run(gpgExecutable_, {"--batch", "--yes", "--delete-secret-and-public-key", fingerprint});
}

GpgProcessResult KeyStore::generatePrivateKey(const std::string& name,
                                              const std::string& email,
                                              const std::string& comment,
                                              const std::string& expires,
                                              const std::string& profileId) const {
    std::string uid = name;
    if (!comment.empty()) {
        uid += " (" + comment + ")";
    }
    if (!email.empty()) {
        uid += " <" + email + ">";
    }
    return GpgProcess::run(gpgExecutable_, generatePrivateKeyArguments(uid, expires, profileId));
}

GpgProcessResult KeyStore::addEncryptionSubkey(const std::string& fingerprint,
                                               const std::string& expires,
                                               const std::string& profileId) const {
    return GpgProcess::run(gpgExecutable_, addEncryptionSubkeyArguments(fingerprint, expires, profileId));
}

GpgProcessResult KeyStore::importOwnerTrust(const std::string& fingerprint, int trustLevel) const {
    std::ostringstream input;
    input << fingerprint << ':' << trustLevel << ":\n";
    return GpgProcess::run(gpgExecutable_, {"--import-ownertrust"}, input.str());
}

std::vector<std::string> KeyStore::generatePrivateKeyArguments(const std::string& uid,
                                                               const std::string& expires,
                                                               const std::string& profileId) {
    std::string algorithm = "default";
    std::string usage = "default";
    if (hasProfileId(profileId, "rsa3072")) {
        algorithm = "rsa3072";
    } else if (hasProfileId(profileId, "rsa4096")) {
        algorithm = "rsa4096";
    } else if (hasProfileId(profileId, "ed25519-cv25519")) {
        algorithm = "ed25519";
        usage = "cert,sign";
    }
    return {"--batch", "--yes", "--no-tty", "--quick-generate-key", uid, algorithm, usage, expires};
}

std::vector<std::string> KeyStore::addEncryptionSubkeyArguments(const std::string& fingerprint,
                                                                const std::string& expires,
                                                                const std::string& profileId) {
    if (hasProfileId(profileId, "ed25519-cv25519")) {
        return {"--batch", "--yes", "--no-tty", "--quick-add-key", fingerprint, "cv25519", "encrypt", expires};
    }
    return {};
}

std::vector<KeyGenerationProfile> KeyStore::availableKeyGenerationProfiles(const std::string& gpgVersionOutput) {
    std::vector<KeyGenerationProfile> profiles;
    profiles.push_back({"default", "Défaut GPG", "Profil recommandé par la version de GPG sélectionnée.", false});
    profiles.push_back({"rsa3072", "RSA 3072", "Clé RSA avec capacités de signature et chiffrement.", false});
    profiles.push_back({"rsa4096", "RSA 4096", "Clé RSA avec capacités de signature et chiffrement.", false});

    auto version = upperAscii(gpgVersionOutput);
    if (version.find("EDDSA") != std::string::npos && version.find("ECDH") != std::string::npos) {
        profiles.push_back({"ed25519-cv25519",
                            "Ed25519 + Curve25519",
                            "Clé Ed25519 pour signer, avec sous-clé Curve25519 pour chiffrer.",
                            true});
    }
    return profiles;
}

std::vector<KeyGenerationProfile> KeyStore::availableKeyGenerationProfiles() const {
    auto result = GpgProcess::run(gpgExecutable_, {"--version"});
    if (!result.success()) {
        return availableKeyGenerationProfiles({});
    }
    return availableKeyGenerationProfiles(result.standardOutput);
}

std::vector<GpgKey> KeyStore::parseColonListing(const std::string& text, bool secretListing) {
    std::vector<GpgKey> keys;
    GpgKey* current = nullptr;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        auto fields = splitColonLine(line);
        auto recordType = fieldAt(fields, 0);
        if (isPrimaryRecord(recordType)) {
            GpgKey key;
            key.type = recordType;
            key.hasSecretKey = secretListing || recordType == "sec";
            key.validity = fieldAt(fields, 1);
            key.keyId = fieldAt(fields, 4);
            key.createdAt = fieldAt(fields, 5);
            key.expiresAt = fieldAt(fields, 6);
            key.capabilities = fieldAt(fields, 11);
            key.revoked = key.validity == "r";
            key.expired = key.validity == "e";
            applyCapabilities(key);
            keys.push_back(key);
            current = &keys.back();
            continue;
        }
        if (!current) {
            continue;
        }
        if (recordType == "uid" && current->uid.empty()) {
            current->validity = current->validity.empty() ? fieldAt(fields, 1) : current->validity;
            current->uid = fieldAt(fields, 9);
            current->email = extractEmail(current->uid);
        } else if (recordType == "fpr" && current->fingerprint.empty()) {
            current->fingerprint = fieldAt(fields, 9);
        } else if (recordType == "sub" || recordType == "ssb") {
            auto capabilities = fieldAt(fields, 11);
            current->capabilities += capabilities;
            current->canEncrypt = current->canEncrypt || hasCapability(capabilities, 'e');
            current->canSign = current->canSign || hasCapability(capabilities, 's');
            current->canCertify = current->canCertify || hasCapability(capabilities, 'c');
            current->canAuthenticate = current->canAuthenticate || hasCapability(capabilities, 'a');
        }
    }
    keys.erase(std::remove_if(keys.begin(), keys.end(), [](const GpgKey& key) {
        return key.fingerprint.empty();
    }), keys.end());
    return keys;
}
