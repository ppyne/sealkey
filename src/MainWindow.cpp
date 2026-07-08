#include "MainWindow.hpp"

#include "AppInfo.hpp"
#include "CryptoService.hpp"
#include "GpgExecutable.hpp"
#include "KeyStore.hpp"
#include "SealKeyModels.hpp"
#include "SealKeyPaths.hpp"
#include "SignatureService.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <ctime>
#include <utility>
#include <vector>

#ifndef SEALKEY_OBJECT_ICONS_SOURCE_DIR
#define SEALKEY_OBJECT_ICONS_SOURCE_DIR ""
#endif

#ifndef SEALKEY_OBJECT_ICONS_INSTALLED_DIR
#define SEALKEY_OBJECT_ICONS_INSTALLED_DIR ""
#endif

namespace {
constexpr int Margin = 14;
constexpr int RowHeight = 28;
constexpr int LabelWidth = 190;
constexpr int ButtonWidth = 92;
constexpr int KeyColumnCount = 6;
constexpr int SignerColumnCount = 5;
constexpr int MinColumnWidth = 45;
constexpr char ResultStyleService = 'A';
constexpr char ResultStyleNormal = 'B';
constexpr char ResultStyleStderr = 'C';
constexpr char ResultStyleExitCode = 'D';
constexpr Fl_Font ResultFont = FL_COURIER;
constexpr Fl_Fontsize ResultFontSize = 12;
constexpr int ListIconSize = 18;

const Fl_Text_Display::Style_Table_Entry ResultStyleTable[] = {
    {FL_BLUE, ResultFont, ResultFontSize, 0, 0},
    {FL_BLACK, ResultFont, ResultFontSize, 0, 0},
    {FL_RED, ResultFont, ResultFontSize, 0, 0},
    {FL_MAGENTA, ResultFont, ResultFontSize, 0, 0},
};

std::string valueOf(Fl_Input* input) {
    return input && input->value() ? input->value() : "";
}

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

enum class SignerStatusIcon : std::intptr_t {
    None = 0,
    Good = 1,
    Bad = 2,
};

std::vector<std::filesystem::path> objectIconDirectories();

std::filesystem::path objectIconPath(const std::string& fileName) {
    for (const auto& directory : objectIconDirectories()) {
        if (directory.empty()) {
            continue;
        }
        std::error_code ec;
        auto path = directory / fileName;
        if (std::filesystem::is_regular_file(path, ec)) {
            return path;
        }
    }
    return {};
}

std::unique_ptr<Fl_Image> loadListIcon(const std::string& baseName) {
    auto path = objectIconPath(baseName + "_32.png");
    if (path.empty()) {
        path = objectIconPath(baseName + "_24.png");
    }
    if (path.empty()) {
        path = objectIconPath(baseName + "_16.png");
    }
    if (path.empty()) {
        return nullptr;
    }

    auto image = std::make_unique<Fl_PNG_Image>(path.string().c_str());
    if (image->w() <= 0 || image->h() <= 0) {
        return nullptr;
    }
    image->scale(ListIconSize, ListIconSize, 1, 1);
    return image;
}

struct ListIcons {
    std::unique_ptr<Fl_Image> publicKey = loadListIcon("key");
    std::unique_ptr<Fl_Image> privateKey = loadListIcon("key-private");
    std::unique_ptr<Fl_Image> seal = loadListIcon("seal");
    std::unique_ptr<Fl_Image> good = loadListIcon("good");
    std::unique_ptr<Fl_Image> bad = loadListIcon("bad");
};

ListIcons& listIcons() {
    static ListIcons icons;
    return icons;
}

void setLastLineIcon(Fl_Hold_Browser* browser, Fl_Image* image) {
    if (browser && image && browser->size() > 0) {
        browser->icon(browser->size(), image);
    }
}

std::filesystem::path executablePath() {
#if defined(__APPLE__)
    std::vector<char> buffer(1024);
    uint32_t size = static_cast<uint32_t>(buffer.size());
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        buffer.resize(size);
        if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
            return {};
        }
    }
    std::error_code ec;
    return std::filesystem::weakly_canonical(buffer.data(), ec);
#elif defined(_WIN32)
    std::vector<char> buffer(MAX_PATH);
    DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size()) {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    if (size == 0) {
        return {};
    }
    return std::filesystem::path(std::string(buffer.data(), size));
#elif defined(__linux__)
    std::vector<char> buffer(4096);
    auto size = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (size <= 0) {
        return {};
    }
    buffer[static_cast<std::size_t>(size)] = '\0';
    return std::filesystem::path(buffer.data());
#else
    return {};
#endif
}

std::vector<std::filesystem::path> objectIconDirectories() {
    std::vector<std::filesystem::path> directories;
    auto executable = executablePath();
    if (!executable.empty()) {
        auto executableDirectory = executable.parent_path();
        directories.push_back(executableDirectory / "icons" / "objets");
#if defined(__APPLE__)
        directories.push_back(executableDirectory.parent_path() / "Resources" / "icons" / "objets");
#endif
    }
    directories.emplace_back(SEALKEY_OBJECT_ICONS_SOURCE_DIR);
    directories.emplace_back(SEALKEY_OBJECT_ICONS_INSTALLED_DIR);
    directories.push_back(std::filesystem::current_path() / "icons" / "objets");
    directories.push_back(std::filesystem::current_path() / "Contents" / "Resources" / "icons" / "objets");
    return directories;
}

bool validEmail(const std::string& email) {
    auto at = email.find('@');
    auto dot = email.find('.', at == std::string::npos ? 0 : at);
    return !email.empty() && at != std::string::npos && at > 0 && dot != std::string::npos && dot > at + 1 &&
           dot + 1 < email.size() && email.find(' ') == std::string::npos;
}

std::string extractEmailFromUid(const std::string& uid) {
    auto start = uid.find('<');
    auto end = uid.find('>', start == std::string::npos ? 0 : start);
    if (start == std::string::npos || end == std::string::npos || end <= start + 1) {
        return {};
    }
    return uid.substr(start + 1, end - start - 1);
}

bool fileExists(const std::string& path) {
    std::error_code ec;
    return !path.empty() && std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
}

bool directoryExists(const std::string& path) {
    std::error_code ec;
    return !path.empty() && std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
}

std::string directoryOf(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

std::string homeDirectory() {
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

std::string existingDirectoryForFileChooser(const std::string& filePath) {
    auto directory = directoryOf(filePath);
    if (directoryExists(directory)) {
        return directory;
    }
    auto home = homeDirectory();
    return directoryExists(home) ? home : std::string{};
}

std::string diagnosticText(const GpgProcessResult& result) {
    std::ostringstream out;
    out << "Code retour : " << result.exitCode;
    if (!result.errorMessage.empty()) {
        out << "\nErreur de lancement : " << result.errorMessage;
    }
    if (!result.standardOutput.empty()) {
        out << "\nSortie standard :\n" << result.standardOutput;
    }
    if (!result.standardError.empty()) {
        out << "\nSortie erreur :\n" << result.standardError;
    }
    return out.str();
}

std::string chronologicalProcessText(const GpgProcessResult& result) {
    std::ostringstream output;
    bool hasOutput = false;
    if (!result.errorMessage.empty()) {
        output << result.errorMessage;
        hasOutput = true;
    } else if (!result.outputChunks.empty()) {
        for (const auto& chunk : result.outputChunks) {
            output << chunk.text;
        }
        hasOutput = true;
    } else if (!result.standardOutput.empty() && !result.standardError.empty()) {
        output << result.standardOutput << "\n" << result.standardError;
        hasOutput = true;
    } else if (!result.standardOutput.empty()) {
        output << result.standardOutput;
        hasOutput = true;
    } else if (!result.standardError.empty()) {
        output << result.standardError;
        hasOutput = true;
    }
    if (hasOutput) {
        output << "\n";
    }
    output << "Code de sortie GPG : " << result.exitCode;
    return output.str();
}

std::string chronologicalProcessStyle(const GpgProcessResult& result) {
    const auto exitCodeLine = std::string("Code de sortie GPG : ") + std::to_string(result.exitCode);
    std::string style;
    bool hasOutput = false;
    if (!result.errorMessage.empty()) {
        style.append(result.errorMessage.size(), ResultStyleStderr);
        hasOutput = true;
    } else if (!result.outputChunks.empty()) {
        for (const auto& chunk : result.outputChunks) {
            style.append(chunk.text.size(), chunk.stream == GpgOutputStream::Stderr ? ResultStyleStderr : ResultStyleNormal);
        }
        hasOutput = true;
    } else if (!result.standardOutput.empty() && !result.standardError.empty()) {
        style.append(result.standardOutput.size(), ResultStyleNormal);
        style += ResultStyleNormal;
        style.append(result.standardError.size(), ResultStyleStderr);
        hasOutput = true;
    } else if (!result.standardOutput.empty()) {
        style.append(result.standardOutput.size(), ResultStyleNormal);
        hasOutput = true;
    } else if (!result.standardError.empty()) {
        style.append(result.standardError.size(), ResultStyleStderr);
        hasOutput = true;
    }
    if (hasOutput) {
        style += ResultStyleNormal;
    }
    style.append(exitCodeLine.size(), ResultStyleExitCode);
    return style;
}

std::string readableGpgError(const std::string& operation, const std::string& file, const GpgProcessResult& result) {
    std::ostringstream out;
    out << operation;
    if (!file.empty()) {
        out << "\nFichier : " << file;
    }
    out << "\n" << diagnosticText(result);
    return out.str();
}

std::string removeDiagnosticSection(std::string text) {
    auto marker = text.find("\n\nDiagnostic GPG :\n");
    if (marker != std::string::npos) {
        text.erase(marker);
    }
    return text;
}

std::string keyLabel(const GpgKey& key) {
    std::ostringstream label;
    label << (key.uid.empty() ? "(identité inconnue)" : key.uid);
    if (!key.keyId.empty()) {
        label << "  [" << key.keyId << "]";
    }
    if (!key.fingerprint.empty()) {
        label << "  " << key.fingerprint;
    }
    if (!key.capabilities.empty()) {
        label << "  cap:" << key.capabilities;
    }
    if (!key.expiresAt.empty()) {
        label << "  exp:" << key.expiresAt;
    }
    if (key.expired) {
        label << "  expirée";
    }
    if (key.revoked) {
        label << "  révoquée";
    }
    return label.str();
}

std::string privateKeyColumnLabel(const GpgKey& key) {
    auto identity = identityFromKey(key);
    std::ostringstream label;
    label << (identity.name.empty() ? "(sans nom)" : identity.name) << '\t'
          << (identity.email.empty() ? "-" : identity.email) << '\t'
          << shortFingerprint(identity.fingerprint) << '\t'
          << identity.fingerprint << '\t'
          << (key.capabilities.empty() ? "-" : key.capabilities) << '\t'
          << (identity.expiresAt.empty() ? "-" : formatGpgTimestampDate(identity.expiresAt));
    return label.str();
}

std::string recipientKeyColumnLabel(const GpgKey& key) {
    auto contact = contactFromKey(key);
    std::ostringstream label;
    label << (contact.name.empty() ? "(sans nom)" : contact.name) << '\t'
          << (contact.email.empty() ? "-" : contact.email) << '\t'
          << shortFingerprint(contact.fingerprint) << '\t'
          << contact.fingerprint << '\t'
          << (key.capabilities.empty() ? "-" : key.capabilities) << '\t'
          << (contact.expiresAt.empty() ? "-" : formatGpgTimestampDate(contact.expiresAt));
    return label.str();
}

std::string yesNo(bool value) {
    return value ? "Oui" : "Non";
}

std::string formattedDateOrDash(const std::string& timestamp) {
    return timestamp.empty() ? "-" : formatGpgTimestampDate(timestamp);
}

void addInfoRow(Fl_Browser* browser, const std::string& label, const std::string& value) {
    browser->add((label + '\t' + (value.empty() ? "-" : value)).c_str());
}

void showKeyInfoDialog(const GpgKey& key) {
    auto* dialog = new Fl_Window(720, 430, "Informations de la clé");
    dialog->set_modal();
    int x = 18;
    int y = 20;
    auto* browser = new Fl_Browser(x, y, dialog->w() - 36, 340);
    static int columnWidths[] = {170, 500, 0};
    browser->column_char('\t');
    browser->column_widths(columnWidths);
    browser->format_char(0);
    addInfoRow(browser, "Type", key.hasSecretKey ? "Clé privée locale" : "Clé publique");
    addInfoRow(browser, "Identité", key.uid);
    addInfoRow(browser, "Email", key.email);
    addInfoRow(browser, "Fingerprint court", shortFingerprint(key.fingerprint));
    addInfoRow(browser, "Fingerprint long", key.fingerprint);
    addInfoRow(browser, "Key ID", key.keyId);
    addInfoRow(browser, "Création", formattedDateOrDash(key.createdAt));
    addInfoRow(browser, "Expiration", formattedDateOrDash(key.expiresAt));
    addInfoRow(browser, "Capacités brutes", key.capabilities);
    addInfoRow(browser, "Signer", yesNo(key.canSign));
    addInfoRow(browser, "Chiffrer", yesNo(key.canEncrypt));
    addInfoRow(browser, "Certifier", yesNo(key.canCertify));
    addInfoRow(browser, "Authentifier", yesNo(key.canAuthenticate));
    addInfoRow(browser, "Révoquée", yesNo(key.revoked));
    addInfoRow(browser, "Expirée", yesNo(key.expired));
    addInfoRow(browser, "Validité GPG", key.validity);
    y += 354;
    auto* closeButton = new Fl_Button(dialog->w() - 118, y, 100, RowHeight, "Fermer");
    closeButton->callback([](Fl_Widget*, void* data) {
        static_cast<Fl_Window*>(data)->hide();
    }, dialog);
    dialog->end();
    dialog->show();
    while (dialog->shown()) {
        Fl::wait();
    }
    delete dialog;
}

struct SignerDisplayInfo {
    std::string name;
    std::string email;
    std::string keyId;
    std::string signedAt;
    std::string status;
};

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream input(text);
    std::string word;
    while (input >> word) {
        words.push_back(word);
    }
    return words;
}

bool allDigits(const std::string& value) {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isdigit(c) != 0;
    });
}

std::string signatureDateFromValidSigWords(const std::vector<std::string>& words) {
    if (words.size() > 3 && allDigits(words[3])) {
        return formatGpgTimestampDate(words[3]);
    }
    if (words.size() > 2) {
        return allDigits(words[2]) ? formatGpgTimestampDate(words[2]) : words[2];
    }
    return {};
}

std::string signatureDateFromErrSigWords(const std::vector<std::string>& words) {
    if (words.size() > 5 && allDigits(words[5])) {
        return formatGpgTimestampDate(words[5]);
    }
    return {};
}

std::string signerStatusLabel(const std::string& status) {
    if (status == "GOODSIG" || status == "VALIDSIG") {
        return "Signature valide";
    }
    if (status == "BADSIG") {
        return "Signature invalide";
    }
    if (status == "NO_PUBKEY") {
        return "Clé publique inconnue";
    }
    if (status == "ERRSIG") {
        return "Signature non vérifiable";
    }
    return status.empty() ? "Signature détectée" : status;
}

SignerStatusIcon signerStatusIcon(const std::string& status) {
    return status == "Signature valide" ? SignerStatusIcon::Good : SignerStatusIcon::Bad;
}

std::vector<SignerDisplayInfo> signerInfoFromGpgText(const std::string& text) {
    std::map<std::string, SignerDisplayInfo> signers;
    std::string lastKey;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        constexpr const char* prefix = "[GNUPG:] ";
        if (line.rfind(prefix, 0) != 0) {
            continue;
        }
        auto payload = line.substr(std::string(prefix).size());
        auto words = splitWords(payload);
        if (words.empty()) {
            continue;
        }
        const auto& record = words.front();
        if ((record == "GOODSIG" || record == "BADSIG") && words.size() >= 2) {
            auto keyId = words[1];
            auto uidStart = payload.find(keyId);
            std::string uid;
            if (uidStart != std::string::npos) {
                uid = trim(payload.substr(uidStart + keyId.size()));
            }
            auto& signer = signers[keyId];
            signer.keyId = keyId;
            signer.name = uid.empty() ? "-" : displayNameFromUid(uid);
            signer.email = extractEmailFromUid(uid);
            signer.status = signerStatusLabel(record);
            lastKey = keyId;
        } else if (record == "VALIDSIG" && words.size() >= 2) {
            auto fingerprint = words[1];
            auto key = lastKey.empty() ? fingerprint : lastKey;
            auto& signer = signers[key];
            signer.keyId = fingerprint;
            if (signer.name.empty()) {
                signer.name = "-";
            }
            signer.signedAt = signatureDateFromValidSigWords(words);
            signer.status = signerStatusLabel(record);
            lastKey = key;
        } else if ((record == "ERRSIG" || record == "NO_PUBKEY") && words.size() >= 2) {
            auto keyId = words[1];
            auto& signer = signers[keyId];
            signer.keyId = keyId;
            signer.name = "-";
            if (record == "ERRSIG") {
                signer.signedAt = signatureDateFromErrSigWords(words);
            }
            signer.status = signerStatusLabel(record);
            lastKey = keyId;
        }
    }

    std::vector<SignerDisplayInfo> output;
    for (auto& [key, signer] : signers) {
        (void)key;
        if (signer.email.empty()) {
            signer.email = "-";
        }
        if (signer.signedAt.empty()) {
            signer.signedAt = "-";
        }
        output.push_back(signer);
    }
    return output;
}

std::string serializeColumnWidths(const int* widths, int count) {
    std::ostringstream output;
    for (int i = 0; i < count; ++i) {
        if (i > 0) {
            output << ',';
        }
        output << widths[i];
    }
    return output.str();
}

void parseColumnWidths(const std::string& text, int* widths, int count) {
    std::istringstream input(text);
    std::string item;
    int index = 0;
    while (index < count && std::getline(input, item, ',')) {
        try {
            int value = std::stoi(trim(item));
            if (value >= MinColumnWidth && value <= 800) {
                widths[index] = value;
            }
        } catch (...) {
        }
        ++index;
    }
    widths[count] = 0;
}

class ColumnHeader : public Fl_Widget {
public:
    ColumnHeader(int x,
                 int y,
                 int w,
                 int h,
                 int* widths,
                 int columnCount,
                 std::vector<std::string> labels,
                 std::function<void()> onResize)
        : Fl_Widget(x, y, w, h),
          widths_(widths),
          columnCount_(columnCount),
          labels_(std::move(labels)),
          onResize_(std::move(onResize)) {}

    void draw() override {
        fl_push_clip(x(), y(), w(), h());
        fl_color(FL_BACKGROUND_COLOR);
        fl_rectf(x(), y(), w(), h());
        fl_color(FL_DARK3);
        fl_rect(x(), y(), w(), h());

        int currentX = x();
        for (int i = 0; i < columnCount_; ++i) {
            fl_color(FL_BLACK);
            fl_font(FL_HELVETICA_BOLD, 12);
            fl_draw(labels_[static_cast<std::size_t>(i)].c_str(), currentX + 4, y(), widths_[i] - 8, h(),
                    FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
            currentX += widths_[i];
            fl_color(FL_DARK3);
            fl_line(currentX, y(), currentX, y() + h());
        }
        fl_pop_clip();
    }

    int handle(int event) override {
        switch (event) {
        case FL_PUSH:
            dragColumn_ = columnBoundaryNear(Fl::event_x());
            lastX_ = Fl::event_x();
            return dragColumn_ >= 0 ? 1 : 0;
        case FL_DRAG:
            if (dragColumn_ < 0) {
                return 0;
            }
            resizeColumns(Fl::event_x() - lastX_);
            lastX_ = Fl::event_x();
            return 1;
        case FL_RELEASE:
            if (dragColumn_ >= 0 && onResize_) {
                onResize_();
            }
            dragColumn_ = -1;
            updateCursor(Fl::event_x());
            return 1;
        case FL_ENTER:
        case FL_MOVE:
            updateCursor(Fl::event_x());
            return 1;
        case FL_LEAVE:
            setDefaultCursor();
            return 1;
        default:
            return Fl_Widget::handle(event);
        }
    }

private:
    int columnBoundaryNear(int eventX) const {
        if (std::abs(eventX - (x() + w())) <= 5) {
            return columnCount_ - 1;
        }
        int boundary = x();
        for (int i = 0; i < columnCount_; ++i) {
            boundary += widths_[i];
            if (std::abs(eventX - boundary) <= 5) {
                return i;
            }
        }
        return -1;
    }

    void resizeColumns(int delta) {
        if (delta == 0 || dragColumn_ < 0) {
            return;
        }
        auto left = dragColumn_;
        if (dragColumn_ == columnCount_ - 1) {
            if (widths_[left] + delta < MinColumnWidth || widths_[left] + delta > 800) {
                return;
            }
            widths_[left] += delta;
            redraw();
            if (onResize_) {
                onResize_();
            }
            return;
        }
        auto right = dragColumn_ + 1;
        if (widths_[left] + delta < MinColumnWidth ||
            widths_[right] - delta < MinColumnWidth) {
            return;
        }
        widths_[left] += delta;
        widths_[right] -= delta;
        redraw();
        if (onResize_) {
            onResize_();
        }
    }

    void updateCursor(int eventX) {
        if (window()) {
            window()->cursor(columnBoundaryNear(eventX) >= 0 ? FL_CURSOR_WE : FL_CURSOR_DEFAULT);
        }
    }

    void setDefaultCursor() {
        if (window()) {
            window()->cursor(FL_CURSOR_DEFAULT);
        }
    }

    int* widths_ = nullptr;
    int columnCount_ = 0;
    std::vector<std::string> labels_;
    std::function<void()> onResize_;
    int dragColumn_ = -1;
    int lastX_ = 0;
};

class SignerBrowser : public Fl_Hold_Browser {
public:
    SignerBrowser(int x, int y, int w, int h)
        : Fl_Hold_Browser(x, y, w, h) {}

protected:
    int item_height(void* item) const override {
        return std::max(Fl_Hold_Browser::item_height(item), ListIconSize + 4);
    }

    void item_draw(void* item, int x, int y, int w, int h) const override {
        const char* text = item_text(item);
        if (!text) {
            return;
        }

        auto columns = splitTabs(text);
        const int* widths = column_widths();
        const bool selected = item_selected(item) != 0;
        const auto normalColor = selected ? fl_contrast(textcolor(), selection_color()) : textcolor();
        const int line = lineno(item);
        const auto statusIcon =
            static_cast<SignerStatusIcon>(reinterpret_cast<std::intptr_t>(data(line)));

        fl_font(textfont(), textsize());
        int currentX = x;
        int remainingW = w;
        for (std::size_t index = 0; index < columns.size() && remainingW > 6; ++index) {
            int columnW = remainingW;
            if (widths && widths[index] > 0) {
                columnW = widths[index];
            }

            fl_push_clip(currentX, y, columnW, h);
            int textX = currentX + 3;
            if (index == 0 && statusIcon != SignerStatusIcon::None) {
                if (auto* icon = listIcons().seal.get()) {
                    icon->draw(currentX + 4, y + (h - icon->h()) / 2);
                    textX += icon->w() + 7;
                }
            } else if (index == 4) {
                Fl_Image* icon = nullptr;
                if (statusIcon == SignerStatusIcon::Good) {
                    icon = listIcons().good.get();
                } else if (statusIcon == SignerStatusIcon::Bad) {
                    icon = listIcons().bad.get();
                }
                if (icon) {
                    icon->draw(currentX + 4, y + (h - icon->h()) / 2);
                    textX += icon->w() + 7;
                }
            }

            fl_color(active_r() ? normalColor : fl_inactive(normalColor));
            fl_draw(columns[index].c_str(), textX, y, std::max(0, currentX + columnW - textX - 3), h,
                    FL_ALIGN_LEFT | FL_ALIGN_CLIP);
            fl_pop_clip();

            currentX += columnW;
            remainingW -= columnW;
            if (!widths || widths[index] == 0) {
                break;
            }
        }
    }

private:
    static std::vector<std::string> splitTabs(const char* text) {
        std::vector<std::string> columns;
        std::istringstream input(text);
        std::string item;
        while (std::getline(input, item, '\t')) {
            columns.push_back(item);
        }
        if (columns.empty()) {
            columns.emplace_back();
        }
        return columns;
    }
};

std::string publicExportFileName(const GpgKey& key) {
    auto display = displayNameFromUid(key.uid);
    std::replace_if(display.begin(), display.end(), [](unsigned char c) {
        return !(std::isalnum(c) || c == '-' || c == '_');
    }, '_');
    if (display.empty()) {
        display = "SealKey";
    }
    return display + "_" + shortFingerprint(key.fingerprint) + "_GPG_PUB.asc";
}

std::string secretExportFileName(const GpgKey& key) {
    auto name = publicExportFileName(key);
    auto marker = name.find("_GPG_PUB.asc");
    if (marker != std::string::npos) {
        name.replace(marker, std::string("_GPG_PUB.asc").size(), "_GPG_PRIVATE.asc");
    }
    return name;
}

struct CreateKeyRequest {
    bool accepted = false;
    std::string name;
    std::string email;
    std::string comment;
    std::string keyProfileId = "default";
    std::string keyProfileLabel = "Défaut GPG";
    bool requiresEncryptionSubkey = false;
    std::string expires = "2y";
    int ownerTrust = 5;
};

std::string validitySuffix(int unitIndex) {
    switch (unitIndex) {
    case 0:
        return "y";
    case 1:
        return "m";
    default:
        return "d";
    }
}

std::string previewExpirationDate(int amount, int unitIndex) {
    if (amount <= 0) {
        return "Aucune expiration";
    }
    auto now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    if (unitIndex == 0) {
        tm.tm_year += amount;
    } else if (unitIndex == 1) {
        tm.tm_mon += amount;
    } else {
        tm.tm_mday += amount;
    }
    auto future = std::mktime(&tm);
    return formatGpgTimestampDate(std::to_string(static_cast<long long>(future)));
}

struct CreateKeyDialogContext {
    Fl_Window* dialog = nullptr;
    CreateKeyRequest* request = nullptr;
    Fl_Int_Input* validityInput = nullptr;
    Fl_Choice* validityUnitChoice = nullptr;
    Fl_Output* validityPreview = nullptr;
};

int validityAmount(const Fl_Int_Input* input) {
    auto value = input && input->value() ? trim(input->value()) : "";
    if (value.empty()) {
        return -1;
    }
    try {
        return std::stoi(value);
    } catch (...) {
        return -1;
    }
}

void updateValidityPreview(CreateKeyDialogContext* context) {
    if (!context || !context->validityPreview || !context->validityInput || !context->validityUnitChoice) {
        return;
    }
    auto amount = validityAmount(context->validityInput);
    if (amount < 0) {
        context->validityPreview->value("Saisir un entier");
        return;
    }
    context->validityPreview->value(previewExpirationDate(amount, context->validityUnitChoice->value()).c_str());
}

CreateKeyRequest showCreateKeyDialog(const std::vector<KeyGenerationProfile>& profiles) {
    CreateKeyRequest request;
    auto* dialog = new Fl_Window(660, 302, "Créer une nouvelle clé privée");
    dialog->set_modal();
    int x = 18;
    int y = 20;
    constexpr int localLabelWidth = 155;
    auto addDialogLabel = [&](const char* text) {
        auto* label = new Fl_Box(x, y, localLabelWidth, RowHeight, text);
        label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    };

    addDialogLabel("Nom");
    auto* nameInput = new Fl_Input(x + localLabelWidth, y, 320, RowHeight);
    y += RowHeight + 10;
    addDialogLabel("Adresse électronique");
    auto* emailInput = new Fl_Input(x + localLabelWidth, y, 320, RowHeight);
    y += RowHeight + 10;
    addDialogLabel("Commentaire");
    auto* commentInput = new Fl_Input(x + localLabelWidth, y, 320, RowHeight);
    y += RowHeight + 10;
    addDialogLabel("Type de clé");
    auto* keyProfileChoice = new Fl_Choice(x + localLabelWidth, y, 300, RowHeight);
    for (const auto& profile : profiles) {
        keyProfileChoice->add(profile.label.c_str());
    }
    keyProfileChoice->value(0);
    y += RowHeight + 10;
    addDialogLabel("Validité");
    auto* validityInput = new Fl_Int_Input(x + localLabelWidth, y, 70, RowHeight);
    validityInput->value("2");
    auto* validityUnitChoice = new Fl_Choice(validityInput->x() + validityInput->w() + 8, y, 105, RowHeight);
    validityUnitChoice->add("années|mois|jours");
    validityUnitChoice->value(0);
    auto* validityPreview = new Fl_Output(validityUnitChoice->x() + validityUnitChoice->w() + 8, y, 160, RowHeight);
    validityPreview->readonly(1);
    y += RowHeight + 10;
    addDialogLabel("Niveau de confiance");
    auto* trustChoice = new Fl_Choice(x + localLabelWidth, y, 220, RowHeight);
    trustChoice->add("Inconnu|Marginal|Complet|Ultime");
    trustChoice->value(3);
    y += RowHeight + 22;
    auto* cancelButton = new Fl_Button(440, y, 90, RowHeight, "Annuler");
    auto* createButton = new Fl_Button(540, y, 90, RowHeight, "Créer");
    CreateKeyDialogContext dialogContext{dialog, &request, validityInput, validityUnitChoice, validityPreview};

    cancelButton->callback([](Fl_Widget*, void* data) {
        static_cast<Fl_Window*>(data)->hide();
    }, dialog);
    createButton->callback([](Fl_Widget*, void* data) {
        auto* context = static_cast<CreateKeyDialogContext*>(data);
        auto amount = validityAmount(context->validityInput);
        if (amount < 0) {
            context->validityPreview->value("Entier obligatoire");
            return;
        }
        context->request->accepted = true;
        context->dialog->hide();
    }, &dialogContext);
    auto previewCallback = [](Fl_Widget*, void* data) {
        updateValidityPreview(static_cast<CreateKeyDialogContext*>(data));
    };
    validityInput->when(FL_WHEN_CHANGED);
    validityInput->callback(previewCallback, &dialogContext);
    validityUnitChoice->callback(previewCallback, &dialogContext);
    updateValidityPreview(&dialogContext);

    dialog->end();
    dialog->show();
    while (dialog->shown()) {
        Fl::wait();
    }
    request.name = trim(valueOf(nameInput));
    request.email = trim(valueOf(emailInput));
    request.comment = trim(valueOf(commentInput));
    auto profileIndex = keyProfileChoice->value();
    if (profileIndex >= 0 && static_cast<std::size_t>(profileIndex) < profiles.size()) {
        request.keyProfileId = profiles[static_cast<std::size_t>(profileIndex)].id;
        request.keyProfileLabel = profiles[static_cast<std::size_t>(profileIndex)].label;
        request.requiresEncryptionSubkey = profiles[static_cast<std::size_t>(profileIndex)].requiresEncryptionSubkey;
    }
    auto amount = validityAmount(validityInput);
    if (amount < 0) {
        request.expires.clear();
    } else if (amount == 0) {
        request.expires = "0";
    } else {
        request.expires = std::to_string(amount) + validitySuffix(validityUnitChoice->value());
    }
    switch (trustChoice->value()) {
    case 1:
        request.ownerTrust = 3;
        break;
    case 2:
        request.ownerTrust = 4;
        break;
    case 3:
        request.ownerTrust = 5;
        break;
    default:
        request.ownerTrust = 0;
        break;
    }
    delete dialog;
    return request;
}

void makeLabel(int x, int y, const char* text) {
    auto* label = new Fl_Box(x, y, LabelWidth - 10, RowHeight, text);
    label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
}

Fl_Text_Display* makeResultDisplay(int x, int y, int w, int h, Fl_Text_Buffer*& buffer, Fl_Text_Buffer*& styleBuffer) {
    buffer = new Fl_Text_Buffer();
    styleBuffer = new Fl_Text_Buffer();
    auto* display = new Fl_Text_Display(x, y, w, h);
    display->textfont(ResultFont);
    display->textsize(ResultFontSize);
    display->buffer(buffer);
    display->highlight_data(styleBuffer, ResultStyleTable, 4, ResultStyleService, nullptr, nullptr);
    return display;
}

bool confirmKeyAction(const char* title,
                      const std::string& question,
                      const std::string& keyText,
                      const std::string& warning,
                      const char* actionLabel) {
    struct DialogState {
        bool accepted = false;
        Fl_Window* dialog = nullptr;
    } state;

    auto* dialog = new Fl_Window(660, warning.empty() ? 210 : 245, title);
    state.dialog = dialog;
    dialog->set_modal();
    int x = 18;
    int y = 20;
    auto* questionBox = new Fl_Box(x, y, dialog->w() - 36, RowHeight, question.c_str());
    questionBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    y += RowHeight + 8;
    auto* keyOutput = new Fl_Multiline_Output(x, y, dialog->w() - 36, 82);
    keyOutput->value(keyText.c_str());
    keyOutput->readonly(1);
    y += 92;
    if (!warning.empty()) {
        auto* warningBox = new Fl_Box(x, y, dialog->w() - 36, RowHeight, warning.c_str());
        warningBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        y += RowHeight + 8;
    }
    auto* cancelButton = new Fl_Button(dialog->w() - 220, y, 95, RowHeight, "Annuler");
    auto* deleteButton = new Fl_Button(dialog->w() - 115, y, 95, RowHeight, actionLabel);
    cancelButton->callback([](Fl_Widget*, void* data) {
        static_cast<DialogState*>(data)->dialog->hide();
    }, &state);
    deleteButton->callback([](Fl_Widget*, void* data) {
        auto* dialogState = static_cast<DialogState*>(data);
        dialogState->accepted = true;
        dialogState->dialog->hide();
    }, &state);
    dialog->end();
    dialog->show();
    while (dialog->shown()) {
        Fl::wait();
    }
    bool accepted = state.accepted;
    delete dialog;
    return accepted;
}
}

MainWindow::MainWindow()
    : Fl_Window(AppSettings{}.window.width,
                AppSettings{}.window.height,
                AppInfo::FullName) {
    settings_ = settingsService_.load();
    settings_.options.encryptedFileExtension =
        SealKeyPaths::normalizeExtension(settings_.options.encryptedFileExtension, "gpg");
    settings_.options.signatureFileExtension =
        SealKeyPaths::normalizeExtension(settings_.options.signatureFileExtension, "sig");
    loadPrivateKeyColumnWidths();
    loadRecipientKeyColumnWidths();
    loadEncryptRecipientColumnWidths();
    loadSignerColumnWidths();
    resize(settings_.window.x >= 0 ? settings_.window.x : x(),
           settings_.window.y >= 0 ? settings_.window.y : y(),
           settings_.window.width,
           settings_.window.height);
    callback(onClose, this);
    buildInterface();
    loadInitialState();
}

MainWindow::~MainWindow() {
    delete encryptResultBuffer_;
    delete encryptResultStyleBuffer_;
    delete decryptResultBuffer_;
    delete decryptResultStyleBuffer_;
    delete signResultBuffer_;
    delete signResultStyleBuffer_;
    delete verifyResultBuffer_;
    delete verifyResultStyleBuffer_;
}

void MainWindow::buildInterface() {
    tabs_ = new Fl_Tabs(Margin, Margin, w() - Margin * 2, h() - Margin * 2);
    tabs_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->updateLastTab();
        window->saveSettings();
    }, this);

    encryptTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Crypter");
    encryptTab_->begin();
    int x = encryptTab_->x() + Margin;
    int y = encryptTab_->y() + Margin;
    int contentWidth = encryptTab_->w() - Margin * 2;
    makeLabel(x, y, "Fichier");
    encryptFileInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - ButtonWidth - 10, RowHeight);
    encryptFileInput_->readonly(1);
    auto* chooseEncrypt = new Fl_Button(encryptFileInput_->x() + encryptFileInput_->w() + 8, y, ButtonWidth, RowHeight, "Choisir");
    chooseEncrypt->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseEncryptFile(); }, this);
    y += RowHeight + 12;
    makeLabel(x, y, "Destinataire");
    encryptRecipientHeader_ = new ColumnHeader(x + LabelWidth,
                                               y,
                                               contentWidth - LabelWidth,
                                               18,
                                               encryptRecipientColumnWidths_.data(),
                                               KeyColumnCount,
                                               {"Nom", "Email", "Court", "Fingerprint", "Cap.", "Expiration"},
                                               [this]() {
                                                   if (encryptRecipientBrowser_) {
                                                       encryptRecipientBrowser_->column_widths(encryptRecipientColumnWidths_.data());
                                                       encryptRecipientBrowser_->redraw();
                                                   }
                                                   saveEncryptRecipientColumnWidths();
                                               });
    y += 20;
    encryptRecipientBrowser_ = new Fl_Hold_Browser(x + LabelWidth, y, contentWidth - LabelWidth, 130);
    encryptRecipientBrowser_->column_char('\t');
    encryptRecipientBrowser_->column_widths(encryptRecipientColumnWidths_.data());
    encryptRecipientBrowser_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        if (auto* key = window->selectedEncryptRecipientKey()) {
            window->recipientsBrowser_->value(window->encryptRecipientBrowser_->value());
            window->settings_.keys.encryptionFingerprint = key->fingerprint;
            window->saveSettings();
        }
        window->updateActions();
    }, this);
    y += 142;
    encryptSignCheck_ = new Fl_Check_Button(x + LabelWidth, y, 250, RowHeight, "Signer avec ma clé préférée");
    encryptSignCheck_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->settings_.options.encryptAndSign = window->encryptSignCheck_->value() != 0;
        window->saveSettings();
        window->updateActions();
    }, this);
    y += RowHeight + 12;
    encryptButton_ = new Fl_Button(x + LabelWidth, y, 130, RowHeight, "Crypter");
    encryptButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executeEncrypt(); }, this);
    y += RowHeight + 12;
    makeResultDisplay(x, y, contentWidth, encryptTab_->h() - (y - encryptTab_->y()) - Margin, encryptResultBuffer_, encryptResultStyleBuffer_);
    encryptTab_->end();

    decryptTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Décrypter");
    decryptTab_->begin();
    x = decryptTab_->x() + Margin;
    y = decryptTab_->y() + Margin;
    contentWidth = decryptTab_->w() - Margin * 2;
    makeLabel(x, y, "Fichier");
    decryptFileInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - ButtonWidth - 10, RowHeight);
    decryptFileInput_->readonly(1);
    auto* chooseDecrypt = new Fl_Button(decryptFileInput_->x() + decryptFileInput_->w() + 8, y, ButtonWidth, RowHeight, "Choisir");
    chooseDecrypt->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseDecryptFile(); }, this);
    y += RowHeight + 12;
    makeLabel(x, y, "Signataires");
    decryptSignersHeader_ = new ColumnHeader(x + LabelWidth,
                                             y,
                                             contentWidth - LabelWidth,
                                             18,
                                             signerColumnWidths_.data(),
                                             SignerColumnCount,
                                             {"Nom", "Email", "Fingerprint", "Date", "Statut"},
                                             [this]() {
                                                 if (decryptSignersBrowser_) {
                                                     decryptSignersBrowser_->column_widths(signerColumnWidths_.data());
                                                     decryptSignersBrowser_->redraw();
                                                 }
                                                 if (verifySignersBrowser_) {
                                                     verifySignersBrowser_->column_widths(signerColumnWidths_.data());
                                                     verifySignersBrowser_->redraw();
                                                 }
                                                 if (verifySignersHeader_) {
                                                     verifySignersHeader_->redraw();
                                                 }
                                                 saveSignerColumnWidths();
                                             });
    y += 20;
    decryptSignersBrowser_ = new SignerBrowser(x + LabelWidth, y, contentWidth - LabelWidth, 100);
    decryptSignersBrowser_->column_char('\t');
    decryptSignersBrowser_->column_widths(signerColumnWidths_.data());
    decryptSignersBrowser_->add("Aucune signature");
    y += 112;
    decryptButton_ = new Fl_Button(x + LabelWidth, y, 130, RowHeight, "Décrypter");
    decryptButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executeDecrypt(); }, this);
    y += RowHeight + 12;
    makeResultDisplay(x, y, contentWidth, decryptTab_->h() - (y - decryptTab_->y()) - Margin, decryptResultBuffer_, decryptResultStyleBuffer_);
    decryptTab_->end();

    signTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Signer");
    signTab_->begin();
    x = signTab_->x() + Margin;
    y = signTab_->y() + Margin;
    contentWidth = signTab_->w() - Margin * 2;
    makeLabel(x, y, "Fichier");
    signFileInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - ButtonWidth - 10, RowHeight);
    signFileInput_->readonly(1);
    auto* chooseSign = new Fl_Button(signFileInput_->x() + signFileInput_->w() + 8, y, ButtonWidth, RowHeight, "Choisir");
    chooseSign->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseSignFile(); }, this);
    y += RowHeight + 12;
    signButton_ = new Fl_Button(x + LabelWidth, y, 130, RowHeight, "Signer");
    signButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executeSign(); }, this);
    y += RowHeight + 12;
    makeResultDisplay(x, y, contentWidth, signTab_->h() - (y - signTab_->y()) - Margin, signResultBuffer_, signResultStyleBuffer_);
    signTab_->end();

    verifyTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Vérifier");
    verifyTab_->begin();
    x = verifyTab_->x() + Margin;
    y = verifyTab_->y() + Margin;
    contentWidth = verifyTab_->w() - Margin * 2;
    makeLabel(x, y, "Fichier");
    verifyFileInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - ButtonWidth - 10, RowHeight);
    verifyFileInput_->readonly(1);
    auto* chooseVerifyFile = new Fl_Button(verifyFileInput_->x() + verifyFileInput_->w() + 8, y, ButtonWidth, RowHeight, "Choisir");
    chooseVerifyFile->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseVerifyFile(); }, this);
    y += RowHeight + 10;
    makeLabel(x, y, "Signature");
    verifySignatureInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - ButtonWidth - 10, RowHeight);
    verifySignatureInput_->readonly(1);
    auto* chooseVerifySig = new Fl_Button(verifySignatureInput_->x() + verifySignatureInput_->w() + 8, y, ButtonWidth, RowHeight, "Choisir");
    chooseVerifySig->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseVerifySignature(); }, this);
    y += RowHeight + 12;
    makeLabel(x, y, "Signataires");
    verifySignersHeader_ = new ColumnHeader(x + LabelWidth,
                                            y,
                                            contentWidth - LabelWidth,
                                            18,
                                            signerColumnWidths_.data(),
                                            SignerColumnCount,
                                            {"Nom", "Email", "Fingerprint", "Date", "Statut"},
                                            [this]() {
                                                if (verifySignersBrowser_) {
                                                    verifySignersBrowser_->column_widths(signerColumnWidths_.data());
                                                    verifySignersBrowser_->redraw();
                                                }
                                                if (decryptSignersBrowser_) {
                                                    decryptSignersBrowser_->column_widths(signerColumnWidths_.data());
                                                    decryptSignersBrowser_->redraw();
                                                }
                                                if (decryptSignersHeader_) {
                                                    decryptSignersHeader_->redraw();
                                                }
                                                saveSignerColumnWidths();
                                            });
    y += 20;
    verifySignersBrowser_ = new SignerBrowser(x + LabelWidth, y, contentWidth - LabelWidth, 90);
    verifySignersBrowser_->column_char('\t');
    verifySignersBrowser_->column_widths(signerColumnWidths_.data());
    verifySignersBrowser_->add("Aucune signature");
    y += 102;
    verifyButton_ = new Fl_Button(x + LabelWidth, y, 130, RowHeight, "Vérifier");
    verifyButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executeVerify(); }, this);
    y += RowHeight + 12;
    makeResultDisplay(x, y, contentWidth, verifyTab_->h() - (y - verifyTab_->y()) - Margin, verifyResultBuffer_, verifyResultStyleBuffer_);
    verifyTab_->end();

    configurationTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Configuration");
    configurationTab_->begin();
    x = configurationTab_->x() + Margin;
    y = configurationTab_->y() + Margin;
    contentWidth = configurationTab_->w() - Margin * 2;
    makeLabel(x, y, "Chemin de l'exécutable GPG");
    gpgPathInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 286, RowHeight);
    gpgPathInput_->when(FL_WHEN_CHANGED);
    gpgPathInput_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->settings_.gpg.executablePath = valueOf(window->gpgPathInput_);
        window->gpgWorks_ = false;
        window->updateStatus("Chemin GPG modifié. Cliquez sur Tester.");
        window->saveSettings();
        window->updateActions();
    }, this);
    auto* chooseGpg = new Fl_Button(gpgPathInput_->x() + gpgPathInput_->w() + 8, y, 86, RowHeight, "Choisir");
    chooseGpg->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseGpgExecutable(); }, this);
    auto* detectGpg = new Fl_Button(chooseGpg->x() + chooseGpg->w() + 8, y, 86, RowHeight, "Détecter");
    detectGpg->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->autoDetectGpg(); }, this);
    auto* testGpgButton = new Fl_Button(detectGpg->x() + detectGpg->w() + 8, y, 86, RowHeight, "Tester");
    testGpgButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->testGpg(); }, this);
    y += RowHeight + 22;
    constexpr int PrivateKeysGroupHeight = 252;
    auto* privateKeysGroup = new Fl_Group(x, y, contentWidth, PrivateKeysGroupHeight, "Mes clés privées");
    privateKeysGroup->box(FL_ENGRAVED_BOX);
    privateKeysGroup->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
    privateKeysGroup->begin();
    int groupX = privateKeysGroup->x() + 12;
    int groupY = privateKeysGroup->y() + 24;
    int groupWidth = privateKeysGroup->w() - 24;
    makeLabel(groupX, groupY, "Mes clés");
    auto browserX = groupX + LabelWidth;
    privateKeyHeader_ = new ColumnHeader(browserX,
                                         groupY,
                                         groupWidth - LabelWidth,
                                         18,
                                         privateKeyColumnWidths_.data(),
                                         KeyColumnCount,
                                         {"Nom", "Email", "Court", "Fingerprint", "Cap.", "Expiration"},
                                         [this]() {
                                             if (myKeyBrowser_) {
                                                 myKeyBrowser_->column_widths(privateKeyColumnWidths_.data());
                                                 myKeyBrowser_->redraw();
                                             }
                                             savePrivateKeyColumnWidths();
                                         });
    groupY += 20;
    myKeyBrowser_ = new Fl_Hold_Browser(browserX, groupY, groupWidth - LabelWidth, 80);
    myKeyBrowser_->column_char('\t');
    myKeyBrowser_->column_widths(privateKeyColumnWidths_.data());
    myKeyBrowser_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->updateActions();
    }, this);
    groupY += 90;
    importPrivateKeyButton_ = new Fl_Button(groupX + LabelWidth, groupY, 150, RowHeight, "Importer clé privée");
    importPrivateKeyButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->importPrivateKey(); }, this);
    auto* createKey = new Fl_Button(importPrivateKeyButton_->x() + importPrivateKeyButton_->w() + 8, groupY, 70, RowHeight, "Créer");
    createKey->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->createPrivateKey(); }, this);
    infoPrivateKeyButton_ = new Fl_Button(createKey->x() + createKey->w() + 8, groupY, 58, RowHeight, "Info");
    infoPrivateKeyButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->showSelectedPrivateKeyInfo(); }, this);
    deletePrivateKeyButton_ = new Fl_Button(infoPrivateKeyButton_->x() + infoPrivateKeyButton_->w() + 8, groupY, 92, RowHeight, "Supprimer");
    deletePrivateKeyButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->deleteSelectedPrivateKey(); }, this);
    groupY += RowHeight + 8;
    setPreferredKeyButton_ = new Fl_Button(groupX + LabelWidth, groupY, 135, RowHeight, "Ma clé préférée");
    setPreferredKeyButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->setPreferredPrivateKey(); }, this);
    exportMyKeyButton_ = new Fl_Button(setPreferredKeyButton_->x() + setPreferredKeyButton_->w() + 8, groupY, 150, RowHeight, "Exporter clé publique");
    exportMyKeyButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->exportMyPublicKey(); }, this);
    exportPrivateKeyButton_ = new Fl_Button(exportMyKeyButton_->x() + exportMyKeyButton_->w() + 8, groupY, 145, RowHeight, "Exporter clé privée");
    exportPrivateKeyButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->exportPrivateKey(); }, this);
    groupY += RowHeight + 8;
    makeLabel(groupX, groupY, "Ma clé préférée");
    preferredKeyOutput_ = new Fl_Output(groupX + LabelWidth, groupY, groupWidth - LabelWidth, RowHeight);
    preferredKeyOutput_->readonly(1);
    privateKeysGroup->end();

    y += PrivateKeysGroupHeight + 22;
    constexpr int RecipientsGroupHeight = 178;
    auto* recipientsGroup = new Fl_Group(x, y, contentWidth, RecipientsGroupHeight, "Mes destinataires");
    recipientsGroup->box(FL_ENGRAVED_BOX);
    recipientsGroup->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
    recipientsGroup->begin();
    groupX = recipientsGroup->x() + 12;
    groupY = recipientsGroup->y() + 24;
    groupWidth = recipientsGroup->w() - 24;
    makeLabel(groupX, groupY, "Destinataires");
    browserX = groupX + LabelWidth;
    recipientKeyHeader_ = new ColumnHeader(browserX,
                                           groupY,
                                           groupWidth - LabelWidth,
                                           18,
                                           recipientKeyColumnWidths_.data(),
                                           KeyColumnCount,
                                           {"Nom", "Email", "Court", "Fingerprint", "Cap.", "Expiration"},
                                           [this]() {
                                               if (recipientsBrowser_) {
                                                   recipientsBrowser_->column_widths(recipientKeyColumnWidths_.data());
                                                   recipientsBrowser_->redraw();
                                               }
                                               saveRecipientKeyColumnWidths();
                                           });
    groupY += 20;
    recipientsBrowser_ = new Fl_Hold_Browser(browserX, groupY, groupWidth - LabelWidth, 76);
    recipientsBrowser_->column_char('\t');
    recipientsBrowser_->column_widths(recipientKeyColumnWidths_.data());
    recipientsBrowser_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->encryptRecipientBrowser_->value(window->recipientsBrowser_->value());
        if (auto* key = window->selectedConfigRecipientKey()) {
            window->settings_.keys.encryptionFingerprint = key->fingerprint;
            window->saveSettings();
        }
        window->updateActions();
    }, this);
    groupY += 86;
    auto* importRecipient = new Fl_Button(groupX + LabelWidth, groupY, 210, RowHeight, "Ajouter la clé d'un destinataire");
    importRecipient->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->importRecipientKey(); }, this);
    exportRecipientButton_ = new Fl_Button(importRecipient->x() + importRecipient->w() + 8, groupY, 210, RowHeight, "Exporter la clé du destinataire");
    exportRecipientButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->exportSelectedRecipientKey(); }, this);
    deleteRecipientButton_ = new Fl_Button(exportRecipientButton_->x() + exportRecipientButton_->w() + 8, groupY, 100, RowHeight, "Supprimer");
    deleteRecipientButton_->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->deleteSelectedRecipientKey(); }, this);
    recipientsGroup->end();

    y += RecipientsGroupHeight + 12;
    makeLabel(x, y, "Extension du fichier chiffré");
    encryptedExtensionInput_ = new Fl_Input(x + LabelWidth, y, 140, RowHeight);
    encryptedExtensionInput_->callback([](Fl_Widget*, void* data) {
        static_cast<MainWindow*>(data)->normalizeAndSaveEncryptedExtension();
    }, this);
    encryptedExtensionInput_->when(FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE);
    y += RowHeight + 10;
    makeLabel(x, y, "Extension du fichier de signature");
    signatureExtensionInput_ = new Fl_Input(x + LabelWidth, y, 140, RowHeight);
    signatureExtensionInput_->callback([](Fl_Widget*, void* data) {
        static_cast<MainWindow*>(data)->normalizeAndSaveSignatureExtension();
    }, this);
    signatureExtensionInput_->when(FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE);
    y += RowHeight + 12;
    makeLabel(x, y, "État GPG");
    gpgStatusOutput_ = new Fl_Output(x + LabelWidth, y, contentWidth - LabelWidth, RowHeight);
    configurationTab_->end();

    tabs_->end();
    resizable(tabs_);
    size_range(1060, 720);
}

void MainWindow::loadInitialState() {
    gpgPathInput_->value(settings_.gpg.executablePath.c_str());
    encryptSignCheck_->value(settings_.options.encryptAndSign ? 1 : 0);
    encryptedExtensionInput_->value(settings_.options.encryptedFileExtension.c_str());
    signatureExtensionInput_->value(settings_.options.signatureFileExtension.c_str());
    testGpg();
    reloadKeys();

    if (settings_.window.lastTab == "encrypt") {
        tabs_->value(encryptTab_);
    } else if (settings_.window.lastTab == "decrypt") {
        tabs_->value(decryptTab_);
    } else if (settings_.window.lastTab == "sign") {
        tabs_->value(signTab_);
    } else if (settings_.window.lastTab == "verify") {
        tabs_->value(verifyTab_);
    } else {
        tabs_->value(configurationTab_);
    }
    updateActions();
}

void MainWindow::saveSettings() {
    settingsService_.save(settings_);
}

void MainWindow::reloadKeys() {
    std::string error;
    auto allKeys = KeyStore(settings_.gpg.executablePath).listMergedKeys(&error);
    recipientKeys_.clear();
    signingKeys_.clear();
    for (const auto& key : allKeys) {
        if (key.canEncrypt && !key.revoked && !key.expired && !key.fingerprint.empty()) {
            recipientKeys_.push_back(key);
        }
        if (key.hasSecretKey && key.canSign && !key.revoked && !key.expired && !key.fingerprint.empty()) {
            signingKeys_.push_back(key);
        }
    }
    if (settings_.keys.signingFingerprint.empty() && signingKeys_.size() == 1) {
        settings_.keys.signingFingerprint = signingKeys_.front().fingerprint;
        saveSettings();
    }
    populateKeyBrowsers();
    restoreKeySelections();
    if (!error.empty()) {
        updateStatus(error);
    }
}

void MainWindow::populateKeyBrowsers() {
    encryptRecipientBrowser_->clear();
    recipientsBrowser_->clear();
    myKeyBrowser_->clear();
    for (const auto& key : recipientKeys_) {
        encryptRecipientBrowser_->add(recipientKeyColumnLabel(key).c_str());
        setLastLineIcon(encryptRecipientBrowser_, listIcons().publicKey.get());
        recipientsBrowser_->add(recipientKeyColumnLabel(key).c_str());
        setLastLineIcon(recipientsBrowser_, listIcons().publicKey.get());
    }
    for (const auto& key : signingKeys_) {
        myKeyBrowser_->add(privateKeyColumnLabel(key).c_str());
        setLastLineIcon(myKeyBrowser_, listIcons().privateKey.get());
    }
}

void MainWindow::restoreKeySelections() {
    for (int i = 0; i < static_cast<int>(recipientKeys_.size()); ++i) {
        if (recipientKeys_[i].fingerprint == settings_.keys.encryptionFingerprint) {
            encryptRecipientBrowser_->value(i + 1);
            recipientsBrowser_->value(i + 1);
            break;
        }
    }
    for (int i = 0; i < static_cast<int>(signingKeys_.size()); ++i) {
        if (signingKeys_[i].fingerprint == settings_.keys.signingFingerprint) {
            break;
        }
    }
    updatePreferredKeyOutput();
}

void MainWindow::updateActions() {
    const bool hasSigner = selectedSigningKey() != nullptr;
    const bool hasListedSigner = selectedListedSigningKey() != nullptr;
    const auto* configRecipient = selectedConfigRecipientKey();
    const bool hasConfigRecipient = configRecipient != nullptr;
    const bool canDeleteConfigRecipient = hasConfigRecipient && !configRecipient->hasSecretKey;
    const bool encryptReady = fileExists(valueOf(encryptFileInput_)) && selectedEncryptRecipientKey() && gpgConfigured();
    const bool signRequested = hasSigner && settings_.options.encryptAndSign;
    encryptSignCheck_->value(signRequested ? 1 : 0);
    encryptSignCheck_->activate();
    if (!hasSigner) {
        encryptSignCheck_->deactivate();
    }
    (encryptReady && (!signRequested || hasSigner)) ? encryptButton_->activate() : encryptButton_->deactivate();
    (fileExists(valueOf(decryptFileInput_)) && gpgConfigured()) ? decryptButton_->activate() : decryptButton_->deactivate();
    (fileExists(valueOf(signFileInput_)) && hasSigner && gpgConfigured()) ? signButton_->activate() : signButton_->deactivate();
    (fileExists(valueOf(verifyFileInput_)) && fileExists(valueOf(verifySignatureInput_)) && gpgConfigured()) ? verifyButton_->activate()
                                                                                                           : verifyButton_->deactivate();
    hasListedSigner ? setPreferredKeyButton_->activate() : setPreferredKeyButton_->deactivate();
    hasListedSigner ? infoPrivateKeyButton_->activate() : infoPrivateKeyButton_->deactivate();
    hasListedSigner ? exportMyKeyButton_->activate() : exportMyKeyButton_->deactivate();
    hasListedSigner ? exportPrivateKeyButton_->activate() : exportPrivateKeyButton_->deactivate();
    hasListedSigner ? deletePrivateKeyButton_->activate() : deletePrivateKeyButton_->deactivate();
    gpgConfigured() ? importPrivateKeyButton_->activate() : importPrivateKeyButton_->deactivate();
    hasConfigRecipient ? exportRecipientButton_->activate() : exportRecipientButton_->deactivate();
    canDeleteConfigRecipient ? deleteRecipientButton_->activate() : deleteRecipientButton_->deactivate();
}

void MainWindow::updateLastTab() {
    auto* selected = tabs_->value();
    if (selected == encryptTab_) {
        settings_.window.lastTab = "encrypt";
    } else if (selected == decryptTab_) {
        settings_.window.lastTab = "decrypt";
    } else if (selected == signTab_) {
        settings_.window.lastTab = "sign";
    } else if (selected == verifyTab_) {
        settings_.window.lastTab = "verify";
    } else {
        settings_.window.lastTab = "configuration";
    }
}

void MainWindow::chooseGpgExecutable() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir l'exécutable GPG");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    auto initialDirectory = existingDirectoryForFileChooser(valueOf(gpgPathInput_));
    if (!initialDirectory.empty()) {
        chooser.directory(initialDirectory.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        settings_.gpg.executablePath = chooser.filename();
        gpgPathInput_->value(settings_.gpg.executablePath.c_str());
        saveSettings();
        testGpg();
    }
}

void MainWindow::autoDetectGpg() {
    auto detected = GpgExecutable::detect();
    if (detected.empty()) {
        updateStatus("GPG n'a pas été détecté automatiquement.");
        return;
    }
    settings_.gpg.executablePath = detected;
    gpgPathInput_->value(detected.c_str());
    saveSettings();
    testGpg();
}

void MainWindow::testGpg() {
    settings_.gpg.executablePath = valueOf(gpgPathInput_);
    auto result = GpgExecutable(settings_.gpg.executablePath).test();
    gpgWorks_ = result.valid;
    if (result.valid) {
        std::istringstream lines(result.versionText);
        std::string firstLine;
        std::getline(lines, firstLine);
        updateStatus("GPG fonctionne : " + firstLine + " (" + settings_.gpg.executablePath + ")");
        saveSettings();
        reloadKeys();
    } else {
        updateStatus(result.errorText.empty() ? "GPG indisponible." : result.errorText);
    }
    updateActions();
}

void MainWindow::chooseEncryptFile() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir le fichier à crypter");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        encryptFileInput_->value(chooser.filename());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        saveSettings();
    }
    updateActions();
}

void MainWindow::executeEncrypt() {
    const auto source = valueOf(encryptFileInput_);
    const auto* recipient = selectedEncryptRecipientKey();
    if (!fileExists(source) || !recipient) {
        return;
    }
    auto destination = SealKeyPaths::appendExtension(source, settings_.options.encryptedFileExtension);
    if (!confirmOverwrite(destination)) {
        return;
    }
    GpgProcessResult result;
    if (encryptSignCheck_->value()) {
        const auto* signer = selectedSigningKey();
        if (!signer) {
            fl_alert("Sélectionnez une clé privée dans Configuration.");
            return;
        }
        result = CryptoService(settings_.gpg.executablePath)
                     .encryptAndSignFile(source, destination, recipient->fingerprint, signer->fingerprint, true);
    } else {
        result = CryptoService(settings_.gpg.executablePath).encryptFile(source, destination, recipient->fingerprint, true);
    }
    if (result.success()) {
        std::ostringstream text;
        text << "Fichier créé : " << destination << "\nDestinataire : " << keyLabel(*recipient)
             << "\nSigné : " << (encryptSignCheck_->value() ? selectedSigningLabel() : "non");
        setResult(encryptResultBuffer_, text.str(), result);
    } else {
        setResult(encryptResultBuffer_, "Le fichier n'a pas pu être chiffré.\nFichier : " + source, result);
    }
}

void MainWindow::chooseDecryptFile() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir le fichier à décrypter");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    const auto filter = std::string("Fichiers chiffrés\t*.") + settings_.options.encryptedFileExtension + "\nTous les fichiers\t*";
    chooser.filter(filter.c_str());
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        decryptFileInput_->value(chooser.filename());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        saveSettings();
        refreshDecryptSigners();
    }
    updateActions();
}

void MainWindow::refreshDecryptSigners() {
    const auto source = valueOf(decryptFileInput_);
    if (!fileExists(source) || !gpgConfigured()) {
        decryptSignersBrowser_->clear();
        decryptSignersBrowser_->add("Aucune signature");
        return;
    }
    auto result = CryptoService(settings_.gpg.executablePath).inspectEncryptedFile(source);
    setSigners(decryptSignersBrowser_, diagnosticText(result));
}

void MainWindow::executeDecrypt() {
    const auto source = valueOf(decryptFileInput_);
    if (!fileExists(source)) {
        return;
    }
    auto destination = SealKeyPaths::removeExtensionForDecrypt(source, settings_.options.encryptedFileExtension);
    if (destination == source) {
        destination += ".decrypted";
    }
    if (!confirmOverwrite(destination)) {
        return;
    }
    auto result = CryptoService(settings_.gpg.executablePath).decryptFile(source, destination);
    if (result.success()) {
        setSigners(decryptSignersBrowser_, diagnosticText(result));
        setResult(decryptResultBuffer_, "Fichier décrypté : " + destination, result);
    } else {
        setSigners(decryptSignersBrowser_, diagnosticText(result));
        setResult(decryptResultBuffer_, "Le fichier n'a pas pu être décrypté.\nFichier : " + source, result);
    }
}

void MainWindow::chooseSignFile() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir le fichier à signer");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        signFileInput_->value(chooser.filename());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        saveSettings();
    }
    updateActions();
}

void MainWindow::executeSign() {
    const auto source = valueOf(signFileInput_);
    const auto* signer = selectedSigningKey();
    if (!fileExists(source) || !signer) {
        return;
    }
    auto signaturePath = SealKeyPaths::signaturePathFor(source, settings_.options.signatureFileExtension);
    if (!confirmOverwrite(signaturePath)) {
        return;
    }
    auto result = SignatureService(settings_.gpg.executablePath).signFileDetached(source, signaturePath, signer->fingerprint);
    if (result.success()) {
        setResult(signResultBuffer_, "Signature créée : " + signaturePath + "\nClé utilisée : " + keyLabel(*signer), result);
    } else {
        setResult(signResultBuffer_, "La signature n'a pas pu être créée.\nFichier : " + source, result);
    }
}

void MainWindow::chooseVerifyFile() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir le fichier original");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        verifyFileInput_->value(chooser.filename());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        auto expected = SealKeyPaths::signaturePathFor(chooser.filename(), settings_.options.signatureFileExtension);
        if (fileExists(expected)) {
            verifySignatureInput_->value(expected.c_str());
        }
        saveSettings();
        refreshVerifySigners();
    }
    updateActions();
}

void MainWindow::chooseVerifySignature() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir la signature");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    const auto filter = std::string("Signatures\t*.") + settings_.options.signatureFileExtension + "\nTous les fichiers\t*";
    chooser.filter(filter.c_str());
    if (directoryExists(settings_.paths.lastSignatureOpenDir)) {
        chooser.directory(settings_.paths.lastSignatureOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        verifySignatureInput_->value(chooser.filename());
        settings_.paths.lastSignatureOpenDir = directoryOf(chooser.filename());
        saveSettings();
        refreshVerifySigners();
    }
    updateActions();
}

void MainWindow::refreshVerifySigners() {
    const auto source = valueOf(verifyFileInput_);
    const auto signature = valueOf(verifySignatureInput_);
    if (!fileExists(source) || !fileExists(signature) || !gpgConfigured()) {
        verifySignersBrowser_->clear();
        verifySignersBrowser_->add("Aucune signature");
        return;
    }
    auto result = SignatureService(settings_.gpg.executablePath).inspectDetachedFile(signature, source);
    setSigners(verifySignersBrowser_, diagnosticText(result));
}

void MainWindow::executeVerify() {
    const auto source = valueOf(verifyFileInput_);
    const auto signature = valueOf(verifySignatureInput_);
    auto result = SignatureService(settings_.gpg.executablePath).verifyDetachedFile(signature, source);
    auto signersResult = SignatureService(settings_.gpg.executablePath).inspectDetachedFile(signature, source);
    auto summary = SignatureService::summarizeVerification(result);
    setSigners(verifySignersBrowser_, diagnosticText(signersResult));
    setResult(verifyResultBuffer_, removeDiagnosticSection(summary.message), result);
}

void MainWindow::importRecipientKey() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Ajouter la clé d'un destinataire");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    chooser.filter("Clés publiques\t*.asc\nClés GPG\t*.gpg\nClés PGP\t*.pgp\nTous les fichiers\t*");
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() != 0 || !chooser.filename()) {
        return;
    }
    auto result = KeyStore(settings_.gpg.executablePath).importPublicKey(chooser.filename());
    if (result.success()) {
        updateStatus("Clé importée depuis : " + std::string(chooser.filename()));
        reloadKeys();
    } else {
        updateStatus(readableGpgError("La clé n'a pas pu être importée.", chooser.filename(), result));
    }
}

void MainWindow::exportSelectedRecipientKey() {
    const auto* key = selectedConfigRecipientKey();
    if (!key) {
        return;
    }
    Fl_Native_File_Chooser chooser;
    chooser.title("Exporter la clé du destinataire");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.preset_file(publicExportFileName(*key).c_str());
    if (directoryExists(settings_.paths.lastKeyExportDir)) {
        chooser.directory(settings_.paths.lastKeyExportDir.c_str());
    }
    if (chooser.show() != 0 || !chooser.filename()) {
        return;
    }
    std::string error;
    auto data = KeyStore(settings_.gpg.executablePath).exportPublicKey(key->fingerprint, &error);
    if (data.empty()) {
        updateStatus("Export impossible : " + error);
        return;
    }
    std::ofstream out(chooser.filename(), std::ios::binary | std::ios::trunc);
    out << data;
    settings_.paths.lastKeyExportDir = directoryOf(chooser.filename());
    saveSettings();
    updateStatus(out ? "Clé publique exportée : " + std::string(chooser.filename()) : "Écriture impossible.");
}

void MainWindow::exportMyPublicKey() {
    const auto* key = selectedListedSigningKey();
    if (!key) {
        return;
    }
    Fl_Native_File_Chooser chooser;
    chooser.title("Exporter ma clé publique");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.preset_file(publicExportFileName(*key).c_str());
    if (directoryExists(settings_.paths.lastKeyExportDir)) {
        chooser.directory(settings_.paths.lastKeyExportDir.c_str());
    }
    if (chooser.show() != 0 || !chooser.filename()) {
        return;
    }
    std::string error;
    auto data = KeyStore(settings_.gpg.executablePath).exportPublicKey(key->fingerprint, &error);
    if (data.empty()) {
        updateStatus("Export impossible : " + error);
        return;
    }
    std::ofstream out(chooser.filename(), std::ios::binary | std::ios::trunc);
    out << data;
    settings_.paths.lastKeyExportDir = directoryOf(chooser.filename());
    saveSettings();
    updateStatus(out ? "Ma clé publique a été exportée : " + std::string(chooser.filename()) : "Écriture impossible.");
}

void MainWindow::importPrivateKey() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Importer une clé privée");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    chooser.filter("Clés privées\t*.asc\nClés GPG\t*.gpg\nClés PGP\t*.pgp\nTous les fichiers\t*");
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() != 0 || !chooser.filename()) {
        return;
    }
    if (!confirmKeyAction("Importer une clé privée",
                          "Importer une clé privée dans le trousseau GPG local ?",
                          chooser.filename(),
                          "N'importez que des clés privées dont vous connaissez l'origine.",
                          "Importer")) {
        return;
    }
    auto result = KeyStore(settings_.gpg.executablePath).importPrivateKey(chooser.filename());
    if (result.success()) {
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        saveSettings();
        updateStatus("Clé privée importée depuis : " + std::string(chooser.filename()));
        reloadKeys();
    } else {
        updateStatus(readableGpgError("La clé privée n'a pas pu être importée.", chooser.filename(), result));
    }
}

void MainWindow::exportPrivateKey() {
    const auto* key = selectedListedSigningKey();
    if (!key) {
        return;
    }
    if (!confirmKeyAction("Exporter une clé privée",
                          "Exporter la clé privée suivante ?",
                          keyLabel(*key),
                          "Le fichier exporté devra être protégé avec soin.",
                          "Exporter")) {
        return;
    }
    Fl_Native_File_Chooser chooser;
    chooser.title("Exporter la clé privée");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.preset_file(secretExportFileName(*key).c_str());
    if (directoryExists(settings_.paths.lastKeyExportDir)) {
        chooser.directory(settings_.paths.lastKeyExportDir.c_str());
    }
    if (chooser.show() != 0 || !chooser.filename()) {
        return;
    }
    std::string error;
    auto data = KeyStore(settings_.gpg.executablePath).exportSecretKey(key->fingerprint, &error);
    if (data.empty()) {
        updateStatus("Export de clé privée impossible : " + error);
        return;
    }
    std::ofstream out(chooser.filename(), std::ios::binary | std::ios::trunc);
    out << data;
    settings_.paths.lastKeyExportDir = directoryOf(chooser.filename());
    saveSettings();
    updateStatus(out ? "Clé privée exportée : " + std::string(chooser.filename()) : "Écriture impossible.");
}

void MainWindow::deleteSelectedPrivateKey() {
    const auto* key = selectedListedSigningKey();
    if (!key) {
        return;
    }
    if (!confirmKeyAction("Supprimer une clé privée",
                          "Supprimer cette clé privée et sa clé publique du trousseau GPG ?",
                          keyLabel(*key),
                          "Cette action est destructive.",
                          "Supprimer")) {
        return;
    }
    auto fingerprint = key->fingerprint;
    auto result = KeyStore(settings_.gpg.executablePath).deleteSecretAndPublicKey(fingerprint);
    if (!result.success()) {
        updateStatus(readableGpgError("La clé privée n'a pas pu être supprimée.", "", result));
        return;
    }
    if (settings_.keys.signingFingerprint == fingerprint) {
        settings_.keys.signingFingerprint.clear();
        saveSettings();
    }
    updateStatus("Clé privée supprimée : " + fingerprint);
    reloadKeys();
}

void MainWindow::deleteSelectedRecipientKey() {
    const auto* key = selectedConfigRecipientKey();
    if (!key) {
        return;
    }
    if (key->hasSecretKey) {
        updateStatus("Suppression refusée : cette clé publique est liée à une clé privée locale.");
        updateActions();
        return;
    }
    if (!confirmKeyAction("Supprimer une clé publique",
                          "Supprimer cette clé publique de destinataire du trousseau GPG ?",
                          keyLabel(*key),
                          "",
                          "Supprimer")) {
        return;
    }
    auto fingerprint = key->fingerprint;
    auto result = KeyStore(settings_.gpg.executablePath).deletePublicKey(fingerprint);
    if (!result.success()) {
        updateStatus(readableGpgError("La clé publique n'a pas pu être supprimée.", "", result));
        return;
    }
    if (settings_.keys.encryptionFingerprint == fingerprint) {
        settings_.keys.encryptionFingerprint.clear();
        saveSettings();
    }
    updateStatus("Clé publique supprimée : " + fingerprint);
    reloadKeys();
}

void MainWindow::showSelectedPrivateKeyInfo() {
    const auto* key = selectedListedSigningKey();
    if (!key) {
        return;
    }
    showKeyInfoDialog(*key);
}

void MainWindow::setPreferredPrivateKey() {
    const auto* key = selectedListedSigningKey();
    if (!key) {
        return;
    }
    settings_.keys.signingFingerprint = key->fingerprint;
    saveSettings();
    updatePreferredKeyOutput();
    updateActions();
    updateStatus("Clé préférée mise à jour : " + keyLabel(*key));
}

void MainWindow::createPrivateKey() {
    KeyStore store(settings_.gpg.executablePath);
    auto profiles = store.availableKeyGenerationProfiles();
    const auto request = showCreateKeyDialog(profiles);
    if (!request.accepted) {
        return;
    }
    if (request.name.empty()) {
        updateStatus("Création annulée : le nom est obligatoire.");
        return;
    }
    if (!validEmail(request.email)) {
        updateStatus("Création annulée : l'adresse électronique est invalide.");
        return;
    }
    if (request.expires.empty()) {
        updateStatus("Création annulée : la validité doit être un entier.");
        return;
    }

    std::set<std::string> beforeFingerprints;
    for (const auto& key : signingKeys_) {
        beforeFingerprints.insert(key.fingerprint);
    }

    auto result = store.generatePrivateKey(request.name, request.email, request.comment, request.expires, request.keyProfileId);
    if (!result.success()) {
        updateStatus(readableGpgError("La clé privée n'a pas pu être créée.", "", result));
        return;
    }

    std::string error;
    auto keysAfterCreation = store.listMergedKeys(&error);
    std::string newFingerprint;
    std::string trustWarning;
    for (const auto& key : keysAfterCreation) {
        if (key.hasSecretKey && beforeFingerprints.find(key.fingerprint) == beforeFingerprints.end()) {
            newFingerprint = key.fingerprint;
            break;
        }
    }
    if (!newFingerprint.empty()) {
        settings_.keys.signingFingerprint = newFingerprint;
        saveSettings();
        if (request.requiresEncryptionSubkey) {
            auto subkeyResult = store.addEncryptionSubkey(newFingerprint, request.expires, request.keyProfileId);
            if (!subkeyResult.success()) {
                trustWarning = readableGpgError("La clé de signature a été créée, mais la sous-clé de chiffrement n'a pas pu être ajoutée.",
                                                "",
                                                subkeyResult);
            }
        }
        if (request.ownerTrust > 0) {
            auto trustResult = store.importOwnerTrust(newFingerprint, request.ownerTrust);
            if (!trustResult.success()) {
                if (!trustWarning.empty()) {
                    trustWarning += "\n\n";
                }
                trustWarning += readableGpgError("La clé a été créée, mais le niveau de confiance n'a pas pu être appliqué.", "", trustResult);
            }
        }
    }
    if (!trustWarning.empty()) {
        updateStatus(trustWarning);
    } else {
        updateStatus(newFingerprint.empty() ? "Clé privée créée. Les listes de clés ont été actualisées."
                                            : "Clé privée " + request.keyProfileLabel + " créée et définie comme préférée : " + newFingerprint);
    }
    reloadKeys();
}

void MainWindow::normalizeAndSaveEncryptedExtension() {
    auto normalized = SealKeyPaths::normalizeExtension(valueOf(encryptedExtensionInput_), "gpg");
    settings_.options.encryptedFileExtension = normalized;
    encryptedExtensionInput_->value(normalized.c_str());
    saveSettings();
}

void MainWindow::normalizeAndSaveSignatureExtension() {
    auto normalized = SealKeyPaths::normalizeExtension(valueOf(signatureExtensionInput_), "sig");
    settings_.options.signatureFileExtension = normalized;
    signatureExtensionInput_->value(normalized.c_str());
    saveSettings();
}

void MainWindow::loadPrivateKeyColumnWidths() {
    parseColumnWidths(settings_.options.privateKeyColumnWidths, privateKeyColumnWidths_.data(), KeyColumnCount);
}

void MainWindow::savePrivateKeyColumnWidths() {
    settings_.options.privateKeyColumnWidths =
        serializeColumnWidths(privateKeyColumnWidths_.data(), KeyColumnCount);
    saveSettings();
}

void MainWindow::loadRecipientKeyColumnWidths() {
    parseColumnWidths(settings_.options.recipientKeyColumnWidths, recipientKeyColumnWidths_.data(), KeyColumnCount);
}

void MainWindow::saveRecipientKeyColumnWidths() {
    settings_.options.recipientKeyColumnWidths =
        serializeColumnWidths(recipientKeyColumnWidths_.data(), KeyColumnCount);
    saveSettings();
}

void MainWindow::loadEncryptRecipientColumnWidths() {
    parseColumnWidths(settings_.options.encryptRecipientColumnWidths, encryptRecipientColumnWidths_.data(), KeyColumnCount);
}

void MainWindow::saveEncryptRecipientColumnWidths() {
    settings_.options.encryptRecipientColumnWidths =
        serializeColumnWidths(encryptRecipientColumnWidths_.data(), KeyColumnCount);
    saveSettings();
}

void MainWindow::loadSignerColumnWidths() {
    parseColumnWidths(settings_.options.signerColumnWidths, signerColumnWidths_.data(), SignerColumnCount);
}

void MainWindow::saveSignerColumnWidths() {
    settings_.options.signerColumnWidths =
        serializeColumnWidths(signerColumnWidths_.data(), SignerColumnCount);
    saveSettings();
}

const GpgKey* MainWindow::selectedEncryptRecipientKey() const {
    int index = encryptRecipientBrowser_->value();
    if (index <= 0 || index > static_cast<int>(recipientKeys_.size())) {
        return nullptr;
    }
    return &recipientKeys_[index - 1];
}

const GpgKey* MainWindow::selectedConfigRecipientKey() const {
    int index = recipientsBrowser_->value();
    if (index <= 0 || index > static_cast<int>(recipientKeys_.size())) {
        return nullptr;
    }
    return &recipientKeys_[index - 1];
}

const GpgKey* MainWindow::selectedListedSigningKey() const {
    int index = myKeyBrowser_->value();
    if (index <= 0 || index > static_cast<int>(signingKeys_.size())) {
        return nullptr;
    }
    return &signingKeys_[index - 1];
}

const GpgKey* MainWindow::selectedSigningKey() const {
    for (const auto& key : signingKeys_) {
        if (key.fingerprint == settings_.keys.signingFingerprint) {
            return &key;
        }
    }
    return nullptr;
}

bool MainWindow::gpgConfigured() const {
    return gpgWorks_ && !settings_.gpg.executablePath.empty();
}

Fl_Text_Buffer* MainWindow::styleBufferFor(Fl_Text_Buffer* buffer) const {
    if (buffer == encryptResultBuffer_) {
        return encryptResultStyleBuffer_;
    }
    if (buffer == decryptResultBuffer_) {
        return decryptResultStyleBuffer_;
    }
    if (buffer == signResultBuffer_) {
        return signResultStyleBuffer_;
    }
    if (buffer == verifyResultBuffer_) {
        return verifyResultStyleBuffer_;
    }
    return nullptr;
}

void MainWindow::setResult(Fl_Text_Buffer* buffer, const std::string& text) {
    if (buffer) {
        buffer->text(text.c_str());
        if (auto* styleBuffer = styleBufferFor(buffer)) {
            styleBuffer->text(std::string(text.size(), ResultStyleService).c_str());
        }
    }
}

void MainWindow::setResult(Fl_Text_Buffer* buffer, const std::string& text, const GpgProcessResult& result) {
    if (!buffer) {
        return;
    }

    auto processText = chronologicalProcessText(result);
    auto processStyle = chronologicalProcessStyle(result);
    std::string output = text;
    std::string style(text.size(), ResultStyleService);
    if (!processText.empty()) {
        if (!output.empty()) {
            output += "\n\nSortie GPG :\n";
            style.append(15, ResultStyleService);
        }
        output += processText;
        style += processStyle;
    }
    buffer->text(output.c_str());
    if (auto* styleBuffer = styleBufferFor(buffer)) {
        styleBuffer->text(style.c_str());
    }
}

void MainWindow::setSigners(Fl_Hold_Browser* browser, const std::string& text) {
    browser->clear();
    auto signers = signerInfoFromGpgText(text);
    if (signers.empty()) {
        browser->add("Aucune signature");
        return;
    }
    for (const auto& signer : signers) {
        std::ostringstream row;
        row << (signer.name.empty() ? "-" : signer.name) << '\t'
            << (signer.email.empty() ? "-" : signer.email) << '\t'
            << (signer.keyId.empty() ? "-" : signer.keyId) << '\t'
            << (signer.signedAt.empty() ? "-" : signer.signedAt) << '\t'
            << (signer.status.empty() ? "Signature détectée" : signer.status);
        const auto status = signer.status.empty() ? "Signature détectée" : signer.status;
        browser->add(row.str().c_str(), reinterpret_cast<void*>(static_cast<std::intptr_t>(signerStatusIcon(status))));
    }
}

void MainWindow::updateStatus(const std::string& message) {
    if (gpgStatusOutput_) {
        gpgStatusOutput_->value(message.c_str());
        flashStatus();
    }
}

void MainWindow::flashStatus() {
    if (!gpgStatusOutput_) {
        return;
    }
    statusFlashRemaining_ = 6;
    gpgStatusOutput_->color(FL_YELLOW);
    gpgStatusOutput_->redraw();
    Fl::remove_timeout(onStatusFlash, this);
    Fl::add_timeout(0.12, onStatusFlash, this);
}

void MainWindow::updatePreferredKeyOutput() {
    if (!preferredKeyOutput_) {
        return;
    }
    const auto* key = selectedSigningKey();
    preferredKeyOutput_->value(key ? keyLabel(*key).c_str() : "Aucune clé préférée");
}

bool MainWindow::confirmOverwrite(const std::string& path) {
    if (!fileExists(path)) {
        return true;
    }
    struct DialogState {
        bool accepted = false;
        Fl_Window* dialog = nullptr;
    } state;

    auto* dialog = new Fl_Window(660, 180, "Confirmer l'écrasement");
    state.dialog = dialog;
    dialog->set_modal();
    int x = 18;
    int y = 20;
    auto* question = new Fl_Box(x, y, dialog->w() - 36, RowHeight, "Le fichier existe déjà. Voulez-vous l'écraser ?");
    question->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    y += RowHeight + 8;
    auto* pathOutput = new Fl_Output(x, y, dialog->w() - 36, RowHeight);
    pathOutput->value(path.c_str());
    pathOutput->readonly(1);
    y += RowHeight + 22;
    auto* cancelButton = new Fl_Button(dialog->w() - 220, y, 95, RowHeight, "Annuler");
    auto* overwriteButton = new Fl_Button(dialog->w() - 115, y, 95, RowHeight, "Écraser");
    cancelButton->callback([](Fl_Widget*, void* data) {
        static_cast<DialogState*>(data)->dialog->hide();
    }, &state);
    overwriteButton->callback([](Fl_Widget*, void* data) {
        auto* dialogState = static_cast<DialogState*>(data);
        dialogState->accepted = true;
        dialogState->dialog->hide();
    }, &state);
    dialog->end();
    dialog->show();
    while (dialog->shown()) {
        Fl::wait();
    }
    const bool accepted = state.accepted;
    delete dialog;
    return accepted;
}

std::string MainWindow::selectedSigningLabel() const {
    const auto* key = selectedSigningKey();
    return key ? keyLabel(*key) : std::string("aucune clé");
}

void MainWindow::onClose(Fl_Widget* widget, void* data) {
    auto* window = static_cast<MainWindow*>(data);
    window->settings_.window.x = window->x();
    window->settings_.window.y = window->y();
    window->settings_.window.width = window->w();
    window->settings_.window.height = window->h();
    window->updateLastTab();
    window->normalizeAndSaveEncryptedExtension();
    window->normalizeAndSaveSignatureExtension();
    window->saveSettings();
    widget->hide();
}

void MainWindow::onStatusFlash(void* data) {
    auto* window = static_cast<MainWindow*>(data);
    if (!window->gpgStatusOutput_ || window->statusFlashRemaining_ <= 0) {
        return;
    }
    --window->statusFlashRemaining_;
    window->gpgStatusOutput_->color((window->statusFlashRemaining_ % 2) == 0 ? FL_YELLOW : FL_WHITE);
    window->gpgStatusOutput_->redraw();
    if (window->statusFlashRemaining_ > 0) {
        Fl::repeat_timeout(0.12, onStatusFlash, data);
    } else {
        window->gpgStatusOutput_->color(FL_WHITE);
        window->gpgStatusOutput_->redraw();
    }
}
