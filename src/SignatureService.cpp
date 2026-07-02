#include "SignatureService.hpp"

#include <utility>

SignatureService::SignatureService(std::string gpgExecutable)
    : gpgExecutable_(std::move(gpgExecutable)) {}

GpgProcessResult SignatureService::signText(const std::string& plainText,
                                            const std::string& signingFingerprint) const {
    return GpgProcess::run(gpgExecutable_, signTextArguments(signingFingerprint), plainText);
}

GpgProcessResult SignatureService::verifyText(const std::string& signedText) const {
    return GpgProcess::run(gpgExecutable_, verifyTextArguments(), signedText);
}

GpgProcessResult SignatureService::signFileDetached(const std::string& sourcePath,
                                                    const std::string& signaturePath,
                                                    const std::string& signingFingerprint) const {
    return GpgProcess::run(gpgExecutable_, signFileDetachedArguments(sourcePath, signaturePath, signingFingerprint));
}

GpgProcessResult SignatureService::verifyDetachedFile(const std::string& signaturePath,
                                                      const std::string& sourcePath) const {
    return GpgProcess::run(gpgExecutable_, verifyDetachedFileArguments(signaturePath, sourcePath));
}

GpgProcessResult SignatureService::verifySignedFile(const std::string& signedFilePath) const {
    return GpgProcess::run(gpgExecutable_, verifySignedFileArguments(signedFilePath));
}

std::vector<std::string> SignatureService::signTextArguments(const std::string& signingFingerprint) {
    return {"--clearsign", "--local-user", signingFingerprint};
}

std::vector<std::string> SignatureService::verifyTextArguments() {
    return {"--verify"};
}

std::vector<std::string> SignatureService::signFileDetachedArguments(const std::string& sourcePath,
                                                                     const std::string& signaturePath,
                                                                     const std::string& signingFingerprint) {
    return {"--armor", "--detach-sign", "--local-user", signingFingerprint, "--output", signaturePath, sourcePath};
}

std::vector<std::string> SignatureService::verifyDetachedFileArguments(const std::string& signaturePath,
                                                                       const std::string& sourcePath) {
    return {"--verify", signaturePath, sourcePath};
}

std::vector<std::string> SignatureService::verifySignedFileArguments(const std::string& signedFilePath) {
    return {"--verify", signedFilePath};
}
