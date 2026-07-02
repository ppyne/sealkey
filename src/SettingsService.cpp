#include "SettingsService.hpp"

#include "AppInfo.hpp"
#include "IniFile.hpp"

#include <cstdlib>
#include <fstream>

#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace {
std::filesystem::path homeDirectory() {
#ifdef _WIN32
    if (const char* userProfile = std::getenv("USERPROFILE")) {
        return userProfile;
    }
#else
    if (const char* home = std::getenv("HOME")) {
        return home;
    }
#endif
    return {};
}

int boundedInt(const IniFile& ini, const std::string& section, const std::string& key, int fallback, int min, int max) {
    int value = ini.getInt(section, key, fallback);
    return value >= min && value <= max ? value : fallback;
}
}

AppSettings SettingsService::load() const {
    AppSettings settings;
    IniFile ini;
    if (!ini.load(preferencesFile())) {
        return settings;
    }

    settings.window.x = boundedInt(ini, "window", "x", settings.window.x, -20000, 20000);
    settings.window.y = boundedInt(ini, "window", "y", settings.window.y, -20000, 20000);
    settings.window.width = boundedInt(ini, "window", "width", settings.window.width, 640, 4000);
    settings.window.height = boundedInt(ini, "window", "height", settings.window.height, 420, 3000);
    settings.window.state = ini.getString("window", "state", settings.window.state);
    if (settings.window.state != "normal" && settings.window.state != "maximized") {
        settings.window.state = "normal";
    }
    settings.window.lastTab = ini.getString("window", "last_tab", settings.window.lastTab);

    settings.gpg.executablePath = ini.getString("gpg", "executable", settings.gpg.executablePath);
    settings.keys.encryptionFingerprint = ini.getString("keys", "encryption_fingerprint", settings.keys.encryptionFingerprint);
    settings.keys.signingFingerprint = ini.getString("keys", "signing_fingerprint", settings.keys.signingFingerprint);

    settings.paths.lastOpenDir = ini.getString("paths", "last_open_dir", settings.paths.lastOpenDir);
    settings.paths.lastSaveDir = ini.getString("paths", "last_save_dir", settings.paths.lastSaveDir);
    settings.paths.lastKeyExportDir = ini.getString("paths", "last_key_export_dir", settings.paths.lastKeyExportDir);
    settings.paths.lastSignatureOpenDir = ini.getString("paths", "last_signature_open_dir", settings.paths.lastSignatureOpenDir);
    settings.paths.lastSignatureSaveDir = ini.getString("paths", "last_signature_save_dir", settings.paths.lastSignatureSaveDir);

    settings.options.armor = ini.getBool("options", "armor", settings.options.armor);
    settings.options.signatureType = ini.getString("options", "signature_type", settings.options.signatureType);
    return settings;
}

bool SettingsService::save(const AppSettings& settings) const {
    std::error_code ec;
    auto directory = configDirectory();
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        return false;
    }
#ifndef _WIN32
    chmod(directory.string().c_str(), 0700);
#endif

    IniFile ini;
    ini.setInt("window", "x", settings.window.x);
    ini.setInt("window", "y", settings.window.y);
    ini.setInt("window", "width", settings.window.width);
    ini.setInt("window", "height", settings.window.height);
    ini.set("window", "state", settings.window.state);
    ini.set("window", "last_tab", settings.window.lastTab);
    ini.set("gpg", "executable", settings.gpg.executablePath);
    ini.set("keys", "encryption_fingerprint", settings.keys.encryptionFingerprint);
    ini.set("keys", "signing_fingerprint", settings.keys.signingFingerprint);
    ini.set("paths", "last_open_dir", settings.paths.lastOpenDir);
    ini.set("paths", "last_save_dir", settings.paths.lastSaveDir);
    ini.set("paths", "last_key_export_dir", settings.paths.lastKeyExportDir);
    ini.set("paths", "last_signature_open_dir", settings.paths.lastSignatureOpenDir);
    ini.set("paths", "last_signature_save_dir", settings.paths.lastSignatureSaveDir);
    ini.setBool("options", "armor", settings.options.armor);
    ini.set("options", "signature_type", settings.options.signatureType);

    auto target = preferencesFile();
    auto temp = target;
    temp += ".tmp";
    if (!ini.save(temp)) {
        return false;
    }
#ifndef _WIN32
    chmod(temp.string().c_str(), 0600);
#endif
    std::filesystem::rename(temp, target, ec);
    if (ec) {
        std::filesystem::remove(temp);
        return false;
    }
    return true;
}

std::filesystem::path SettingsService::configDirectory() const {
#ifdef _WIN32
    if (const char* appData = std::getenv("APPDATA")) {
        return std::filesystem::path(appData) / AppInfo::ProjectId;
    }
    return homeDirectory() / "AppData" / "Roaming" / AppInfo::ProjectId;
#elif defined(__APPLE__)
    return homeDirectory() / "Library" / "Application Support" / AppInfo::ProjectId;
#else
    if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(xdgConfig) / AppInfo::ProjectId;
    }
    return homeDirectory() / ".config" / AppInfo::ProjectId;
#endif
}

std::filesystem::path SettingsService::preferencesFile() const {
    return configDirectory() / AppInfo::PreferencesFile;
}
