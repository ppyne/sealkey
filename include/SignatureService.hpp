#pragma once

#include "GpgProcess.hpp"

#include <string>
#include <vector>

class SignatureService {
public:
    explicit SignatureService(std::string gpgExecutable);

    GpgProcessResult signText(const std::string& plainText, const std::string& signingFingerprint) const;
    GpgProcessResult verifyText(const std::string& signedText) const;

    static std::vector<std::string> signTextArguments(const std::string& signingFingerprint);
    static std::vector<std::string> verifyTextArguments();

private:
    std::string gpgExecutable_;
};
