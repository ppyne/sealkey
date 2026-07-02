#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>

class IniFile {
public:
    bool load(const std::filesystem::path& path);
    bool save(const std::filesystem::path& path) const;
    bool parse(const std::string& text);
    std::string serialize() const;

    std::optional<std::string> get(const std::string& section, const std::string& key) const;
    std::string getString(const std::string& section, const std::string& key, std::string fallback) const;
    int getInt(const std::string& section, const std::string& key, int fallback) const;
    bool getBool(const std::string& section, const std::string& key, bool fallback) const;

    void set(const std::string& section, const std::string& key, const std::string& value);
    void setInt(const std::string& section, const std::string& key, int value);
    void setBool(const std::string& section, const std::string& key, bool value);

private:
    using Section = std::map<std::string, std::string>;
    std::map<std::string, Section> data_;
};
