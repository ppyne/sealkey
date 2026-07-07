#pragma once

#include <filesystem>
#include <string>

struct WindowSettings {
    int x = -1;
    int y = -1;
    int width = 980;
    int height = 720;
    std::string state = "normal";
    std::string lastTab = "configuration";
};

struct GpgSettings {
    std::string executablePath;
};

struct KeySettings {
    std::string encryptionFingerprint;
    std::string signingFingerprint;
};

struct PathSettings {
    std::string lastOpenDir;
    std::string lastSaveDir;
    std::string lastKeyExportDir;
    std::string lastSignatureOpenDir;
    std::string lastSignatureSaveDir;
};

struct OptionSettings {
    bool armor = true;
    bool encryptAndSign = false;
    std::string signatureType = "detached_ascii";
    std::string encryptedFileExtension = "gpg";
    std::string signatureFileExtension = "sig";
    std::string privateKeyColumnWidths = "110,155,85,285,70,85";
    std::string recipientKeyColumnWidths = "110,155,85,285,70,85";
};

struct AppSettings {
    WindowSettings window;
    GpgSettings gpg;
    KeySettings keys;
    PathSettings paths;
    OptionSettings options;
};

class SettingsService {
public:
    AppSettings load() const;
    bool save(const AppSettings& settings) const;

    std::filesystem::path configDirectory() const;
    std::filesystem::path preferencesFile() const;
};
