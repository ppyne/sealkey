#include "SealKeyPaths.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {
std::string trim(std::string value) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char c) {
                    return !isSpace(c);
                }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char c) {
                    return !isSpace(c);
                }).base(),
                value.end());
    return value;
}

bool endsWith(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}
}

namespace SealKeyPaths {
std::string normalizeExtension(std::string value, const std::string& fallback) {
    value = trim(std::move(value));
    while (!value.empty() && value.front() == '.') {
        value.erase(value.begin());
    }
    value = trim(std::move(value));
    return value.empty() ? fallback : value;
}

std::string appendExtension(const std::string& sourcePath, const std::string& extension) {
    return sourcePath + "." + normalizeExtension(extension, "gpg");
}

std::string removeExtensionForDecrypt(const std::string& encryptedPath, const std::string& extension) {
    const auto normalized = "." + normalizeExtension(extension, "gpg");
    if (endsWith(encryptedPath, normalized) && encryptedPath.size() > normalized.size()) {
        return encryptedPath.substr(0, encryptedPath.size() - normalized.size());
    }
    const auto path = std::filesystem::path(encryptedPath);
    auto parent = path.parent_path();
    auto filename = path.filename().string();
    auto candidate = filename + ".decrypted";
    return parent.empty() ? candidate : (parent / candidate).string();
}

std::string signaturePathFor(const std::string& sourcePath, const std::string& extension) {
    return sourcePath + "." + normalizeExtension(extension, "sig");
}
}
