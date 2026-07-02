#pragma once

#include "GpgProcess.hpp"

#include <string>
#include <vector>

class CryptoService {
public:
    explicit CryptoService(std::string gpgExecutable);

    GpgProcessResult encryptText(const std::string& plainText, const std::string& recipientFingerprint) const;
    GpgProcessResult decryptText(const std::string& encryptedText) const;

    static std::vector<std::string> encryptTextArguments(const std::string& recipientFingerprint, bool armor = true);
    static std::vector<std::string> decryptTextArguments();

private:
    std::string gpgExecutable_;
};
