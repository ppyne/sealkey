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

std::vector<std::string> SignatureService::signTextArguments(const std::string& signingFingerprint) {
    return {"--clearsign", "--local-user", signingFingerprint};
}

std::vector<std::string> SignatureService::verifyTextArguments() {
    return {"--verify"};
}
