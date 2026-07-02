#pragma once

#include "GpgProcess.hpp"

#include <string>
#include <vector>

class SignatureService {
public:
    explicit SignatureService(std::string gpgExecutable);

    GpgProcessResult signText(const std::string& plainText, const std::string& signingFingerprint) const;
    GpgProcessResult verifyText(const std::string& signedText) const;
    GpgProcessResult signFileDetached(const std::string& sourcePath,
                                      const std::string& signaturePath,
                                      const std::string& signingFingerprint) const;
    GpgProcessResult verifyDetachedFile(const std::string& signaturePath, const std::string& sourcePath) const;
    GpgProcessResult verifySignedFile(const std::string& signedFilePath) const;

    static std::vector<std::string> signTextArguments(const std::string& signingFingerprint);
    static std::vector<std::string> verifyTextArguments();
    static std::vector<std::string> signFileDetachedArguments(const std::string& sourcePath,
                                                             const std::string& signaturePath,
                                                             const std::string& signingFingerprint);
    static std::vector<std::string> verifyDetachedFileArguments(const std::string& signaturePath,
                                                                const std::string& sourcePath);
    static std::vector<std::string> verifySignedFileArguments(const std::string& signedFilePath);

private:
    std::string gpgExecutable_;
};
