#pragma once

#include <string>

namespace SealKeyPaths {
std::string normalizeExtension(std::string value, const std::string& fallback);
std::string appendExtension(const std::string& sourcePath, const std::string& extension);
std::string removeExtensionForDecrypt(const std::string& encryptedPath, const std::string& extension);
std::string signaturePathFor(const std::string& sourcePath, const std::string& extension);
}
