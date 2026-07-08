#pragma once

#include "GpgKey.hpp"
#include "GpgProcess.hpp"

#include <string>
#include <vector>

struct KeyStoreResult {
    std::vector<GpgKey> keys;
    std::string errorText;
    int exitCode = 0;

    bool success() const { return errorText.empty() && exitCode == 0; }
};

struct KeyGenerationProfile {
    std::string id;
    std::string label;
    std::string description;
    bool requiresEncryptionSubkey = false;
};

class KeyStore {
public:
    explicit KeyStore(std::string gpgExecutable);

    KeyStoreResult listPublicKeys() const;
    KeyStoreResult listSecretKeys() const;
    std::vector<GpgKey> listMergedKeys(std::string* errorText = nullptr) const;
    std::string exportPublicKey(const std::string& fingerprint, std::string* errorText = nullptr) const;
    std::string exportSecretKey(const std::string& fingerprint, std::string* errorText = nullptr) const;
    GpgProcessResult importPublicKey(const std::string& filePath) const;
    GpgProcessResult importPrivateKey(const std::string& filePath) const;
    GpgProcessResult deletePublicKey(const std::string& fingerprint) const;
    GpgProcessResult deleteSecretAndPublicKey(const std::string& fingerprint) const;
    GpgProcessResult generatePrivateKey(const std::string& name,
                                        const std::string& email,
                                        const std::string& comment,
                                        const std::string& expires,
                                        const std::string& profileId) const;
    GpgProcessResult addEncryptionSubkey(const std::string& fingerprint,
                                         const std::string& expires,
                                         const std::string& profileId) const;
    GpgProcessResult importOwnerTrust(const std::string& fingerprint, int trustLevel) const;

    static std::vector<std::string> generatePrivateKeyArguments(const std::string& uid,
                                                                const std::string& expires,
                                                                const std::string& profileId);
    static std::vector<std::string> addEncryptionSubkeyArguments(const std::string& fingerprint,
                                                                 const std::string& expires,
                                                                 const std::string& profileId);
    static std::vector<KeyGenerationProfile> availableKeyGenerationProfiles(const std::string& gpgVersionOutput);
    std::vector<KeyGenerationProfile> availableKeyGenerationProfiles() const;
    static std::vector<GpgKey> parseColonListing(const std::string& text, bool secretListing);

private:
    std::string gpgExecutable_;
};
