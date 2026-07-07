#pragma once

#include "GpgKey.hpp"

#include <string>
#include <vector>

struct GpgIdentity {
    std::string name;
    std::string email;
    std::string fingerprint;
    std::string keyId;
    bool canSign = false;
    bool canDecrypt = false;
    std::string createdAt;
    std::string expiresAt;
};

struct GpgContact {
    std::string name;
    std::string email;
    std::string fingerprint;
    std::string keyId;
    bool canEncrypt = false;
    bool canVerify = false;
    std::string trustLevel;
    std::string createdAt;
    std::string expiresAt;
};

enum class SealKeyOperationType {
    Encrypt,
    Decrypt,
    Sign,
    Verify,
    EncryptAndSign,
    ImportPublicKey,
    ExportPublicKey,
    PreparePackage
};

struct SealKeyOperation {
    SealKeyOperationType type = SealKeyOperationType::Encrypt;
    std::vector<std::string> inputFiles;
    std::string outputPath;
    std::string signaturePath;
    std::string recipientFingerprint;
    std::string signingFingerprint;
    bool includePublicKey = false;
    bool armor = true;
};

struct OperationResult {
    bool success = false;
    std::string userMessage;
    std::string technicalMessage;
    std::vector<std::string> outputFiles;
    std::vector<std::string> warnings;
    std::vector<std::string> suggestedNextActions;
};

GpgIdentity identityFromKey(const GpgKey& key);
GpgContact contactFromKey(const GpgKey& key);
std::string shortFingerprint(const std::string& fingerprint);
std::string displayNameFromUid(const std::string& uid);
