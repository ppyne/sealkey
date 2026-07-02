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

GpgProcessResult CryptoService::encryptFile(const std::string& sourcePath,
                                            const std::string& destinationPath,
                                            const std::string& recipientFingerprint,
                                            bool armor) const {
    return GpgProcess::run(gpgExecutable_, encryptFileArguments(sourcePath, destinationPath, recipientFingerprint, armor));
}

GpgProcessResult CryptoService::decryptFile(const std::string& sourcePath,
                                            const std::string& destinationPath) const {
    return GpgProcess::run(gpgExecutable_, decryptFileArguments(sourcePath, destinationPath));
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

std::vector<std::string> CryptoService::encryptFileArguments(const std::string& sourcePath,
                                                             const std::string& destinationPath,
                                                             const std::string& recipientFingerprint,
                                                             bool armor) {
    std::vector<std::string> args = {"--yes", "--batch"};
    if (armor) {
        args.emplace_back("--armor");
    }
    args.emplace_back("--encrypt");
    args.emplace_back("--recipient");
    args.emplace_back(recipientFingerprint);
    args.emplace_back("--output");
    args.emplace_back(destinationPath);
    args.emplace_back(sourcePath);
    return args;
}

std::vector<std::string> CryptoService::decryptFileArguments(const std::string& sourcePath,
                                                             const std::string& destinationPath) {
    return {"--yes", "--decrypt", "--output", destinationPath, sourcePath};
}
