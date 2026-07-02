#include "GpgExecutable.hpp"

#include "GpgProcess.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>

namespace {
bool executableExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !std::filesystem::is_directory(path, ec);
}

std::vector<std::string> splitPathList(const std::string& pathList) {
    std::vector<std::string> paths;
#ifdef _WIN32
    constexpr char separator = ';';
    constexpr const char* executableName = "gpg.exe";
#else
    constexpr char separator = ':';
    constexpr const char* executableName = "gpg";
#endif
    std::stringstream stream(pathList);
    std::string item;
    while (std::getline(stream, item, separator)) {
        if (!item.empty()) {
            paths.push_back((std::filesystem::path(item) / executableName).string());
        }
    }
    return paths;
}
}

GpgExecutable::GpgExecutable(std::string path) : path_(std::move(path)) {}

const std::string& GpgExecutable::path() const {
    return path_;
}

void GpgExecutable::setPath(std::string path) {
    path_ = std::move(path);
}

bool GpgExecutable::empty() const {
    return path_.empty();
}

GpgTestResult GpgExecutable::test() const {
    GpgTestResult testResult;
    if (path_.empty()) {
        testResult.errorText = "Aucun exécutable gpg sélectionné.";
        return testResult;
    }
    auto result = GpgProcess::run(path_, {"--version"});
    testResult.exitCode = result.exitCode;
    testResult.versionText = result.standardOutput;
    testResult.errorText = result.errorMessage.empty() ? result.standardError : result.errorMessage;
    testResult.valid = result.success() && result.standardOutput.find("gpg") != std::string::npos;
    return testResult;
}

std::string GpgExecutable::detect() {
    for (const auto& candidate : candidatePaths()) {
        if (candidate == "gpg" || executableExists(candidate)) {
            GpgExecutable executable(candidate);
            if (executable.test().valid) {
                return candidate;
            }
        }
    }
    return {};
}

std::vector<std::string> GpgExecutable::candidatePaths() {
    std::vector<std::string> candidates;
#ifdef _WIN32
    candidates.emplace_back("C:\\Program Files (x86)\\GnuPG\\bin\\gpg.exe");
    candidates.emplace_back("C:\\Program Files\\GnuPG\\bin\\gpg.exe");
#elif defined(__APPLE__)
    candidates.emplace_back("/opt/local/bin/gpg");
    candidates.emplace_back("/opt/homebrew/bin/gpg");
    candidates.emplace_back("/usr/local/bin/gpg");
    candidates.emplace_back("/usr/bin/gpg");
#else
    candidates.emplace_back("/usr/bin/gpg");
    candidates.emplace_back("/usr/local/bin/gpg");
    candidates.emplace_back("/bin/gpg");
#endif
    if (const char* path = std::getenv("PATH")) {
        auto pathCandidates = splitPathList(path);
        candidates.insert(candidates.end(), pathCandidates.begin(), pathCandidates.end());
    }
    candidates.emplace_back("gpg");
    return candidates;
}
