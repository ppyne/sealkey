#pragma once

#include "GpgProcess.hpp"

#include <string>
#include <vector>

class CryptoService {
public:
    explicit CryptoService(std::string gpgExecutable);

    GpgProcessResult encryptText(const std::string& plainText, const std::string& recipientFingerprint) const;
    GpgProcessResult decryptText(const std::string& encryptedText) const;
    GpgProcessResult encryptFile(const std::string& sourcePath,
                                 const std::string& destinationPath,
                                 const std::string& recipientFingerprint,
                                 bool armor = true) const;
    GpgProcessResult decryptFile(const std::string& sourcePath, const std::string& destinationPath) const;

    static std::vector<std::string> encryptTextArguments(const std::string& recipientFingerprint, bool armor = true);
    static std::vector<std::string> decryptTextArguments();
    static std::vector<std::string> encryptFileArguments(const std::string& sourcePath,
                                                         const std::string& destinationPath,
                                                         const std::string& recipientFingerprint,
                                                         bool armor = true);
    static std::vector<std::string> decryptFileArguments(const std::string& sourcePath,
                                                         const std::string& destinationPath);

private:
    std::string gpgExecutable_;
};
