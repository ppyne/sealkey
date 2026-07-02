#include "CryptoService.hpp"

#include <utility>

CryptoService::CryptoService(std::string gpgExecutable)
    : gpgExecutable_(std::move(gpgExecutable)) {}

GpgProcessResult CryptoService::encryptText(const std::string& plainText,
                                            const std::string& recipientFingerprint) const {
    return GpgProcess::run(gpgExecutable_, encryptTextArguments(recipientFingerprint, true), plainText);
}

GpgProcessResult CryptoService::decryptText(const std::string& encryptedText) const {
    return GpgProcess::run(gpgExecutable_, decryptTextArguments(), encryptedText);
}

std::vector<std::string> CryptoService::encryptTextArguments(const std::string& recipientFingerprint, bool armor) {
    std::vector<std::string> args;
    if (armor) {
        args.emplace_back("--armor");
    }
    args.emplace_back("--encrypt");
    args.emplace_back("--recipient");
    args.emplace_back(recipientFingerprint);
    return args;
}

std::vector<std::string> CryptoService::decryptTextArguments() {
    return {"--decrypt"};
}
