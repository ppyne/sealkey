#include "IniFile.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {
std::string trim(std::string value) {
    auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}
}

bool IniFile::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parse(buffer.str());
}

bool IniFile::save(const std::filesystem::path& path) const {
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        return false;
    }
    output << serialize();
    return static_cast<bool>(output);
}

bool IniFile::parse(const std::string& text) {
    data_.clear();
    std::istringstream input(text);
    std::string section;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        auto cleaned = trim(line);
        if (cleaned.empty() || cleaned.front() == '#' || cleaned.front() == ';') {
            continue;
        }
        if (cleaned.front() == '[' && cleaned.back() == ']') {
            section = trim(cleaned.substr(1, cleaned.size() - 2));
            continue;
        }
        auto equal = cleaned.find('=');
        if (equal == std::string::npos) {
            continue;
        }
        auto key = trim(cleaned.substr(0, equal));
        auto value = trim(cleaned.substr(equal + 1));
        if (!section.empty() && !key.empty()) {
            data_[section][key] = value;
        }
    }
    return true;
}

std::string IniFile::serialize() const {
    std::ostringstream output;
    bool firstSection = true;
    for (const auto& [section, values] : data_) {
        if (!firstSection) {
            output << '\n';
        }
        firstSection = false;
        output << '[' << section << "]\n";
        for (const auto& [key, value] : values) {
            output << key << '=' << value << '\n';
        }
    }
    return output.str();
}

std::optional<std::string> IniFile::get(const std::string& section, const std::string& key) const {
    auto sectionIt = data_.find(section);
    if (sectionIt == data_.end()) {
        return std::nullopt;
    }
    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return std::nullopt;
    }
    return keyIt->second;
}

std::string IniFile::getString(const std::string& section, const std::string& key, std::string fallback) const {
    auto value = get(section, key);
    return value ? *value : std::move(fallback);
}

int IniFile::getInt(const std::string& section, const std::string& key, int fallback) const {
    auto value = get(section, key);
    if (!value) {
        return fallback;
    }
    try {
        std::size_t consumed = 0;
        int parsed = std::stoi(*value, &consumed);
        return consumed == value->size() ? parsed : fallback;
    } catch (...) {
        return fallback;
    }
}

bool IniFile::getBool(const std::string& section, const std::string& key, bool fallback) const {
    auto value = get(section, key);
    if (!value) {
        return fallback;
    }
    auto lowered = *value;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on") {
        return true;
    }
    if (lowered == "false" || lowered == "0" || lowered == "no" || lowered == "off") {
        return false;
    }
    return fallback;
}

void IniFile::set(const std::string& section, const std::string& key, const std::string& value) {
    data_[section][key] = value;
}

void IniFile::setInt(const std::string& section, const std::string& key, int value) {
    set(section, key, std::to_string(value));
}

void IniFile::setBool(const std::string& section, const std::string& key, bool value) {
    set(section, key, value ? "true" : "false");
}
