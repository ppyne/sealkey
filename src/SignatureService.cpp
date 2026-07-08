#include "SignatureService.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <utility>

namespace {
std::string lowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string combinedOutput(const GpgProcessResult& result) {
    if (!result.errorMessage.empty()) {
        return result.errorMessage;
    }
    if (!result.standardError.empty() && !result.standardOutput.empty()) {
        return result.standardOutput + "\n" + result.standardError;
    }
    return result.standardError.empty() ? result.standardOutput : result.standardError;
}

std::string findFingerprint(const std::string& text) {
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        auto marker = line.find("fingerprint");
        if (marker == std::string::npos) {
            marker = line.find("empreinte");
        }
        if (marker == std::string::npos) {
            continue;
        }
        auto equal = line.find('=', marker);
        auto colon = line.find(':', marker);
        auto separator = equal == std::string::npos ? colon : equal;
        if (separator != std::string::npos && colon != std::string::npos) {
            separator = std::min(separator, colon);
        }
        auto start = separator == std::string::npos ? marker + 11 : separator + 1;
        std::string candidate;
        for (std::size_t i = start; i < line.size(); ++i) {
            unsigned char c = static_cast<unsigned char>(line[i]);
            if (std::isxdigit(c)) {
                candidate.push_back(static_cast<char>(std::toupper(c)));
            }
        }
        if (candidate.size() >= 32) {
            return candidate;
        }
    }
    return {};
}
}

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

GpgProcessResult SignatureService::inspectDetachedFile(const std::string& signaturePath,
                                                       const std::string& sourcePath) const {
    return GpgProcess::run(gpgExecutable_, inspectDetachedFileArguments(signaturePath, sourcePath));
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
    return {"--yes", "--no-tty", "--armor", "--detach-sign", "--local-user", signingFingerprint, "--output", signaturePath, sourcePath};
}

std::vector<std::string> SignatureService::verifyDetachedFileArguments(const std::string& signaturePath,
                                                                       const std::string& sourcePath) {
    return {"--verify", signaturePath, sourcePath};
}

std::vector<std::string> SignatureService::inspectDetachedFileArguments(const std::string& signaturePath,
                                                                        const std::string& sourcePath) {
    return {"--status-fd", "1", "--verify", signaturePath, sourcePath};
}

std::vector<std::string> SignatureService::verifySignedFileArguments(const std::string& signedFilePath) {
    return {"--verify", signedFilePath};
}

VerificationSummary SignatureService::summarizeVerification(const GpgProcessResult& result) {
    VerificationSummary summary;
    const auto text = combinedOutput(result);
    const auto lowered = lowerCopy(text);
    summary.valid = result.exitCode == 0 &&
                    (lowered.find("good signature") != std::string::npos ||
                     lowered.find("bonne signature") != std::string::npos);
    summary.unknownKey = lowered.find("no public key") != std::string::npos ||
                         lowered.find("can't check signature") != std::string::npos ||
                         lowered.find("impossible de vérifier") != std::string::npos;
    summary.expiredKey = lowered.find("expired") != std::string::npos ||
                         lowered.find("expir") != std::string::npos;
    summary.revokedKey = lowered.find("revoked") != std::string::npos ||
                         lowered.find("révo") != std::string::npos ||
                         lowered.find("revo") != std::string::npos;
    summary.untrustedKey = lowered.find("not certified") != std::string::npos ||
                           lowered.find("untrusted") != std::string::npos ||
                           lowered.find("not trusted") != std::string::npos ||
                           lowered.find("pas certifiée") != std::string::npos;
    summary.fingerprint = findFingerprint(text);

    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        auto loweredLine = lowerCopy(line);
        if (loweredLine.find("good signature") != std::string::npos ||
            loweredLine.find("bonne signature") != std::string::npos ||
            loweredLine.find("bad signature") != std::string::npos ||
            loweredLine.find("mauvaise signature") != std::string::npos) {
            summary.signer = line;
            break;
        }
    }

    std::ostringstream message;
    if (summary.valid) {
        message << "Signature valide.";
    } else {
        message << "Signature invalide ou non vérifiable.";
    }
    if (summary.unknownKey) {
        message << "\nClé du signataire inconnue.";
    }
    if (summary.expiredKey) {
        message << "\nAttention : clé ou signature expirée.";
    }
    if (summary.revokedKey) {
        message << "\nAttention : clé révoquée.";
    }
    if (summary.untrustedKey) {
        message << "\nAttention : signature cryptographiquement valide, mais confiance non établie.";
    }
    if (!summary.fingerprint.empty()) {
        message << "\nEmpreinte : " << summary.fingerprint;
    }
    if (!summary.signer.empty()) {
        message << "\n" << summary.signer;
    }
    if (!text.empty()) {
        message << "\n\nDiagnostic GPG :\n" << text;
    }
    summary.message = message.str();
    return summary;
}
