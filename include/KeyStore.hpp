#pragma once

#include "GpgKey.hpp"

#include <string>
#include <vector>

struct KeyStoreResult {
    std::vector<GpgKey> keys;
    std::string errorText;
    int exitCode = 0;

    bool success() const { return errorText.empty() && exitCode == 0; }
};

class KeyStore {
public:
    explicit KeyStore(std::string gpgExecutable);

    KeyStoreResult listPublicKeys() const;
    KeyStoreResult listSecretKeys() const;
    std::vector<GpgKey> listMergedKeys(std::string* errorText = nullptr) const;
    std::string exportPublicKey(const std::string& fingerprint, std::string* errorText = nullptr) const;

    static std::vector<GpgKey> parseColonListing(const std::string& text, bool secretListing);

private:
    std::string gpgExecutable_;
};
