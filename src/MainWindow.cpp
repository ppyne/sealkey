#include "MainWindow.hpp"

#include "AppInfo.hpp"
#include "CryptoService.hpp"
#include "GpgExecutable.hpp"
#include "KeyStore.hpp"
#include "SealKeyModels.hpp"
#include "SignatureService.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/fl_ask.H>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
constexpr int Margin = 12;
constexpr int LabelWidth = 120;
constexpr int RowHeight = 28;

std::string keyLabel(const GpgKey& key) {
    std::ostringstream label;
    label << (key.uid.empty() ? "(UID indisponible)" : key.uid);
    if (!key.fingerprint.empty()) {
        label << "  [" << key.fingerprint << "]";
    }
    if (key.expired) {
        label << "  expirée";
    }
    if (key.revoked) {
        label << "  révoquée";
    }
    if (key.validity != "f" && key.validity != "u") {
        label << "  confiance limitée";
    }
    return label.str();
}

std::string keyWarningText(const GpgKey& key) {
    std::ostringstream warning;
    if (key.expired) {
        warning << "Cette clé est expirée.\n";
    }
    if (key.revoked) {
        warning << "Cette clé est révoquée.\n";
    }
    if (key.validity != "f" && key.validity != "u") {
        warning << "La confiance de cette clé n'est pas pleinement établie.\n";
    }
    return warning.str();
}

std::string keyDetailText(const GpgKey& key, const std::string& emptyText) {
    if (key.fingerprint.empty()) {
        return emptyText;
    }
    auto warnings = keyWarningText(key);
    if (warnings.empty()) {
        return key.fingerprint;
    }
    return key.fingerprint + "  -  " + warnings.substr(0, warnings.find('\n'));
}

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream output;
    output << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return output.str();
}

std::string directoryOf(const std::string& path) {
    if (path.empty()) {
        return {};
    }
    std::error_code ec;
    auto directory = std::filesystem::path(path).parent_path();
    return directory.empty() ? std::string{} : directory.string();
}

bool directoryExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
}

bool fileExists(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
}

std::string defaultFileNameForOperation(const std::string& source, int operation) {
    if (source.empty()) {
        return operation == 2 ? "signature.asc" : "sealkey-output";
    }
    std::filesystem::path sourcePath(source);
    if (operation == 0) {
        return (sourcePath.filename().string() + ".asc");
    }
    if (operation == 1) {
        auto stem = sourcePath.stem().string();
        return stem.empty() ? "decrypted-output" : stem;
    }
    if (operation == 2) {
        return (sourcePath.filename().string() + ".sig.asc");
    }
    return "sealkey-output";
}

std::string defaultProtectedFileName(const std::string& source, bool encrypted) {
    if (source.empty()) {
        return encrypted ? "fichier-protege.gpg" : "signature.asc";
    }
    auto name = std::filesystem::path(source).filename().string();
    return encrypted ? name + ".gpg" : name + ".sig.asc";
}

std::string defaultDecryptedFileName(const std::string& source) {
    if (source.empty()) {
        return "fichier-ouvert";
    }
    auto path = std::filesystem::path(source);
    auto name = path.filename().string();
    for (const std::string suffix : {".gpg", ".pgp", ".asc"}) {
        if (name.size() > suffix.size() && name.substr(name.size() - suffix.size()) == suffix) {
            return name.substr(0, name.size() - suffix.size());
        }
    }
    return name + ".opened";
}

std::string fileKindForPath(const std::string& path) {
    auto extension = std::filesystem::path(path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (extension == ".gpg" || extension == ".pgp") {
        return "Fichier probablement chiffré";
    }
    if (extension == ".sig") {
        return "Signature détachée";
    }
    if (extension == ".asc") {
        return "ASCII armor : signature, clé publique ou fichier chiffré";
    }
    return "Type non reconnu automatiquement";
}

bool fileContainsMarker(const std::string& path, const std::string& marker) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    std::string data(4096, '\0');
    input.read(data.data(), static_cast<std::streamsize>(data.size()));
    data.resize(static_cast<std::size_t>(input.gcount()));
    return data.find(marker) != std::string::npos;
}

std::string makePackageDirectoryName() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream output;
    output << "SealKey_export_" << std::put_time(&tm, "%Y-%m-%d_%H%M%S");
    return output.str();
}

std::string diagnosticText(const GpgProcessResult& result);

std::string readableGpgError(const GpgProcessResult& result) {
    auto text = diagnosticText(result);
    auto lowered = text;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (lowered.find("no secret key") != std::string::npos || lowered.find("secret key not available") != std::string::npos) {
        return "Aucune clé privée compatible n'a été trouvée.";
    }
    if (lowered.find("no public key") != std::string::npos || lowered.find("can't check signature") != std::string::npos) {
        return "Impossible de vérifier la signature : la clé publique de l'expéditeur n'est pas connue.";
    }
    if (lowered.find("bad passphrase") != std::string::npos || lowered.find("cancelled") != std::string::npos) {
        return "La phrase de passe a été refusée ou l'opération a été annulée.";
    }
    if (lowered.find("bad signature") != std::string::npos) {
        return "Signature invalide. Le fichier a peut-être été modifié, ou la signature ne correspond pas à ce fichier.";
    }
    if (lowered.find("permission denied") != std::string::npos) {
        return "Permission refusée pour lire ou écrire un fichier.";
    }
    return text.empty() ? "L'opération GPG a échoué." : text;
}

void setOperationResult(Fl_Text_Buffer* buffer, const OperationResult& result) {
    std::ostringstream text;
    text << result.userMessage;
    if (!result.outputFiles.empty()) {
        text << "\n\nFichiers produits :";
        for (const auto& file : result.outputFiles) {
            text << "\n- " << file;
        }
    }
    if (!result.warnings.empty()) {
        text << "\n\nAvertissements :";
        for (const auto& warning : result.warnings) {
            text << "\n- " << warning;
        }
    }
    if (!result.suggestedNextActions.empty()) {
        text << "\n\nActions proposées :";
        for (const auto& action : result.suggestedNextActions) {
            text << "\n- " << action;
        }
    }
    if (!result.technicalMessage.empty()) {
        text << "\n\nDétails techniques GPG :\n" << result.technicalMessage;
    }
    buffer->text(text.str().c_str());
}

std::string bufferText(Fl_Text_Buffer* buffer) {
    if (!buffer) {
        return {};
    }
    char* raw = buffer->text();
    if (!raw) {
        return {};
    }
    std::string text(raw);
    std::free(raw);
    return text;
}

std::string diagnosticText(const GpgProcessResult& result) {
    if (!result.errorMessage.empty()) {
        return result.errorMessage;
    }
    if (!result.standardError.empty()) {
        return result.standardError;
    }
    return result.standardOutput;
}

bool windowLooksVisible(int x, int y, int width, int height) {
    if (x < 0 || y < 0 || width <= 0 || height <= 0) {
        return false;
    }
    int screenCount = Fl::screen_count();
    for (int i = 0; i < screenCount; ++i) {
        int sx = 0;
        int sy = 0;
        int sw = 0;
        int sh = 0;
        Fl::screen_xywh(sx, sy, sw, sh, i);
        const int overlapLeft = std::max(x, sx);
        const int overlapTop = std::max(y, sy);
        const int overlapRight = std::min(x + width, sx + sw);
        const int overlapBottom = std::min(y + height, sy + sh);
        if (overlapRight - overlapLeft >= 80 && overlapBottom - overlapTop >= 80) {
            return true;
        }
    }
    return false;
}
}

MainWindow::MainWindow()
    : Fl_Window(980, 720, AppInfo::FullName), settings_(settingsService_.load()) {
    buildInterface();
    loadInitialState();
    callback(onClose, this);
}

MainWindow::~MainWindow() {
    delete sendResultBuffer_;
    delete receiveResultBuffer_;
    delete packageResultBuffer_;
    delete textSourceBuffer_;
    delete textResultBuffer_;
    delete logBuffer_;
}

void MainWindow::buildInterface() {
    begin();
    tabs_ = new Fl_Tabs(Margin, Margin, w() - Margin * 2, h() - Margin * 2);
    tabs_->callback([](Fl_Widget*, void* data) {
        static_cast<MainWindow*>(data)->updateLastTab();
    }, this);

    homeTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Accueil");
    homeTab_->begin();
    int x = homeTab_->x() + Margin;
    int y = homeTab_->y() + Margin;
    int contentWidth = homeTab_->w() - Margin * 2;
    new Fl_Box(x, y, contentWidth, RowHeight, "Que voulez-vous faire ?");
    y += RowHeight + 12;
    auto protectButton = new Fl_Button(x, y, 280, 40, "Protéger un fichier pour quelqu'un");
    protectButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->goToTab(static_cast<MainWindow*>(data)->sendTab_); }, this);
    auto openButton = new Fl_Button(x + 300, y, 240, 40, "Ouvrir un fichier protégé");
    openButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->goToTab(static_cast<MainWindow*>(data)->receiveTab_); }, this);
    y += 52;
    auto signButton = new Fl_Button(x, y, 280, 36, "Signer un fichier");
    signButton->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->goToTab(window->sendTab_);
        window->sendActionChoice_->value(1);
    }, this);
    auto verifyButton = new Fl_Button(x + 300, y, 240, 36, "Vérifier une signature");
    verifyButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->goToTab(static_cast<MainWindow*>(data)->receiveTab_); }, this);
    y += 48;
    auto packageButton = new Fl_Button(x, y, 280, 36, "Préparer un dossier d'envoi");
    packageButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->goToTab(static_cast<MainWindow*>(data)->packageTab_); }, this);
    auto importButton = new Fl_Button(x + 300, y, 240, 36, "Importer une clé publique");
    importButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->importPublicKey(); }, this);
    y += 48;
    auto exportHomeButton = new Fl_Button(x, y, 280, 36, "Exporter ma clé publique");
    exportHomeButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->exportEncryptionPublicKey(); }, this);
    auto settingsButton = new Fl_Button(x + 300, y, 115, 36, "Paramètres");
    settingsButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->goToTab(static_cast<MainWindow*>(data)->configurationTab_); }, this);
    auto keysButton = new Fl_Button(x + 425, y, 115, 36, "Clés");
    keysButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->goToTab(static_cast<MainWindow*>(data)->keysTab_); }, this);
    y += 54;
    new Fl_Box(x, y, LabelWidth, RowHeight, "État");
    homeStatusOutput_ = new Fl_Output(x + LabelWidth, y, contentWidth - LabelWidth, RowHeight);
    homeTab_->end();

    sendTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Envoyer");
    sendTab_->begin();
    x = sendTab_->x() + Margin;
    y = sendTab_->y() + Margin;
    contentWidth = sendTab_->w() - Margin * 2;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Fichier");
    sendSourceInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 110, RowHeight);
    auto sendSourceButton = new Fl_Button(sendSourceInput_->x() + sendSourceInput_->w() + 8, y, 100, RowHeight, "Choisir");
    sendSourceButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseSendSource(); }, this);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Action");
    sendActionChoice_ = new Fl_Choice(x + LabelWidth, y, 240, RowHeight);
    sendActionChoice_->add("Chiffrer");
    sendActionChoice_->add("Signer");
    sendActionChoice_->add("Chiffrer et signer");
    sendActionChoice_->value(2);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Destinataire");
    sendRecipientBrowser_ = new Fl_Hold_Browser(x + LabelWidth, y, contentWidth - LabelWidth, 120);
    y += 132;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Sortie");
    sendOutputInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 110, RowHeight);
    auto sendOutputButton = new Fl_Button(sendOutputInput_->x() + sendOutputInput_->w() + 8, y, 100, RowHeight, "Choisir");
    sendOutputButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseSendOutput(); }, this);
    y += RowHeight + 12;
    auto sendExecuteButton = new Fl_Button(x + LabelWidth, y, 180, RowHeight, "Voir résumé et exécuter");
    sendExecuteButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executeSendOperation(); }, this);
    y += RowHeight + 10;
    sendResultBuffer_ = new Fl_Text_Buffer();
    sendResultDisplay_ = new Fl_Text_Display(x, y, contentWidth, sendTab_->h() - y + sendTab_->y() - Margin);
    sendResultDisplay_->buffer(sendResultBuffer_);
    sendTab_->end();

    receiveTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Recevoir");
    receiveTab_->begin();
    x = receiveTab_->x() + Margin;
    y = receiveTab_->y() + Margin;
    contentWidth = receiveTab_->w() - Margin * 2;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Fichier reçu");
    receiveFileInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 110, RowHeight);
    auto receiveFileButton = new Fl_Button(receiveFileInput_->x() + receiveFileInput_->w() + 8, y, 100, RowHeight, "Choisir");
    receiveFileButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseReceiveFile(); }, this);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Détection");
    receiveDetectedOutput_ = new Fl_Output(x + LabelWidth, y, contentWidth - LabelWidth, RowHeight);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Sortie");
    receiveOutputInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 110, RowHeight);
    auto receiveOutputButton = new Fl_Button(receiveOutputInput_->x() + receiveOutputInput_->w() + 8, y, 100, RowHeight, "Choisir");
    receiveOutputButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseReceiveOutput(); }, this);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Original");
    receiveOriginalInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 110, RowHeight);
    auto receiveOriginalButton = new Fl_Button(receiveOriginalInput_->x() + receiveOriginalInput_->w() + 8, y, 100, RowHeight, "Choisir");
    receiveOriginalButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseReceiveOriginal(); }, this);
    y += RowHeight + 12;
    auto receiveExecuteButton = new Fl_Button(x + LabelWidth, y, 180, RowHeight, "Analyser / ouvrir");
    receiveExecuteButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executeReceiveOperation(); }, this);
    y += RowHeight + 10;
    receiveResultBuffer_ = new Fl_Text_Buffer();
    receiveResultDisplay_ = new Fl_Text_Display(x, y, contentWidth, receiveTab_->h() - y + receiveTab_->y() - Margin);
    receiveResultDisplay_->buffer(receiveResultBuffer_);
    receiveTab_->end();

    packageTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Préparer");
    packageTab_->begin();
    x = packageTab_->x() + Margin;
    y = packageTab_->y() + Margin;
    contentWidth = packageTab_->w() - Margin * 2;
    auto addPackageButton = new Fl_Button(x, y, 150, RowHeight, "Ajouter fichier");
    addPackageButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->addPackageFile(); }, this);
    auto clearPackageButton = new Fl_Button(x + 160, y, 130, RowHeight, "Vider la liste");
    clearPackageButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->clearPackageFiles(); }, this);
    y += RowHeight + 8;
    packageFilesBrowser_ = new Fl_Hold_Browser(x, y, contentWidth, 120);
    y += 132;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Action");
    packageActionChoice_ = new Fl_Choice(x + LabelWidth, y, 240, RowHeight);
    packageActionChoice_->add("Signer");
    packageActionChoice_->add("Chiffrer");
    packageActionChoice_->add("Chiffrer et signer");
    packageActionChoice_->value(2);
    packageIncludePublicKeyCheck_ = new Fl_Check_Button(x + LabelWidth + 260, y, 260, RowHeight, "Inclure ma clé publique");
    packageIncludePublicKeyCheck_->value(1);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Destinataire");
    packageRecipientBrowser_ = new Fl_Hold_Browser(x + LabelWidth, y, contentWidth - LabelWidth, 90);
    y += 102;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Dossier sortie");
    packageOutputDirInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 110, RowHeight);
    auto packageOutputButton = new Fl_Button(packageOutputDirInput_->x() + packageOutputDirInput_->w() + 8, y, 100, RowHeight, "Choisir");
    packageOutputButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->choosePackageOutputDir(); }, this);
    y += RowHeight + 12;
    auto packageExecuteButton = new Fl_Button(x + LabelWidth, y, 170, RowHeight, "Créer le dossier");
    packageExecuteButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executePackageOperation(); }, this);
    y += RowHeight + 10;
    packageResultBuffer_ = new Fl_Text_Buffer();
    packageResultDisplay_ = new Fl_Text_Display(x, y, contentWidth, packageTab_->h() - y + packageTab_->y() - Margin);
    packageResultDisplay_->buffer(packageResultBuffer_);
    packageTab_->end();

    configurationTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Configuration");
    configurationTab_->begin();
    x = configurationTab_->x() + Margin;
    y = configurationTab_->y() + Margin;
    contentWidth = configurationTab_->w() - Margin * 2;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Exécutable gpg");
    gpgPathInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 330, RowHeight);
    auto chooseButton = new Fl_Button(gpgPathInput_->x() + gpgPathInput_->w() + 8, y, 100, RowHeight, "Choisir");
    chooseButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseGpgExecutable(); }, this);
    auto detectButton = new Fl_Button(chooseButton->x() + chooseButton->w() + 8, y, 100, RowHeight, "Détecter");
    detectButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->autoDetectGpg(); }, this);
    auto testButton = new Fl_Button(detectButton->x() + detectButton->w() + 8, y, 90, RowHeight, "Tester");
    testButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->testGpg(); }, this);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Résultat");
    gpgStatusOutput_ = new Fl_Output(x + LabelWidth, y, contentWidth - LabelWidth - 140, RowHeight);
    auto reloadButton = new Fl_Button(gpgStatusOutput_->x() + gpgStatusOutput_->w() + 8, y, 130, RowHeight, "Recharger clés");
    reloadButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->reloadKeys(); }, this);
    configurationTab_->end();

    keysTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Clés");
    keysTab_->begin();
    x = keysTab_->x() + Margin;
    y = keysTab_->y() + Margin;
    contentWidth = keysTab_->w() - Margin * 2;
    int browserWidth = (contentWidth - 16) / 2;
    new Fl_Box(x, y, browserWidth, RowHeight, "Clé de chiffrement");
    new Fl_Box(x + browserWidth + 16, y, browserWidth, RowHeight, "Clé de signature");
    y += RowHeight;
    encryptionBrowser_ = new Fl_Hold_Browser(x, y, browserWidth, 300);
    encryptionBrowser_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->settings_.keys.encryptionFingerprint = window->selectedEncryptionFingerprint();
        auto* key = window->selectedEncryptionKey();
        window->encryptionFingerprintOutput_->value(key ? keyDetailText(*key, "Aucune clé de chiffrement sélectionnée").c_str()
                                                        : "Aucune clé de chiffrement sélectionnée");
        if (key && !keyWarningText(*key).empty()) {
            window->appendLog("Avertissement sur la clé de chiffrement sélectionnée.");
        }
        window->saveSettings();
    }, this);
    signingBrowser_ = new Fl_Hold_Browser(x + browserWidth + 16, y, browserWidth, 300);
    signingBrowser_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->settings_.keys.signingFingerprint = window->selectedSigningFingerprint();
        auto* key = window->selectedSigningKey();
        window->signingFingerprintOutput_->value(key ? keyDetailText(*key, "Aucune clé de signature sélectionnée").c_str()
                                                     : "Aucune clé de signature sélectionnée");
        if (key && !keyWarningText(*key).empty()) {
            window->appendLog("Avertissement sur la clé de signature sélectionnée.");
        }
        window->saveSettings();
    }, this);
    y += 312;
    new Fl_Box(x, y, 110, RowHeight, "Empreinte");
    encryptionFingerprintOutput_ = new Fl_Output(x + 110, y, browserWidth - 110, RowHeight);
    new Fl_Box(x + browserWidth + 16, y, 110, RowHeight, "Empreinte");
    signingFingerprintOutput_ = new Fl_Output(x + browserWidth + 126, y, browserWidth - 110, RowHeight);
    y += RowHeight + 12;
    auto exportButton = new Fl_Button(x, y, 220, RowHeight, "Exporter la clé publique");
    exportButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->exportEncryptionPublicKey(); }, this);
    keysTab_->end();

    textTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Texte");
    textTab_->begin();
    x = textTab_->x() + Margin;
    y = textTab_->y() + Margin;
    contentWidth = textTab_->w() - Margin * 2;
    textOperationChoice_ = new Fl_Choice(x, y, 150, RowHeight, "Opération");
    textOperationChoice_->add("Chiffrer");
    textOperationChoice_->add("Déchiffrer");
    textOperationChoice_->add("Signer");
    textOperationChoice_->add("Vérifier");
    textOperationChoice_->value(0);
    auto executeTextButton = new Fl_Button(x + 230, y, 100, RowHeight, "Exécuter");
    executeTextButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executeTextOperation(); }, this);
    auto copyTextButton = new Fl_Button(executeTextButton->x() + executeTextButton->w() + 8, y, 130, RowHeight, "Copier résultat");
    copyTextButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->copyTextResult(); }, this);
    auto clearClipboardButton = new Fl_Button(copyTextButton->x() + copyTextButton->w() + 8, y, 150, RowHeight, "Vider presse-papiers");
    clearClipboardButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->clearClipboard(); }, this);
    auto clearTextButton = new Fl_Button(clearClipboardButton->x() + clearClipboardButton->w() + 8, y, 80, RowHeight, "Effacer");
    clearTextButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->clearTextBuffers(); }, this);
    auto saveTextButton = new Fl_Button(clearTextButton->x() + clearTextButton->w() + 8, y, 150, RowHeight, "Enregistrer résultat");
    saveTextButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->saveTextResult(); }, this);
    y += RowHeight + 12;
    int editorHeight = (textTab_->h() - y + textTab_->y() - Margin * 2 - 28) / 2;
    new Fl_Box(x, y, 80, RowHeight, "Source");
    y += RowHeight;
    textSourceBuffer_ = new Fl_Text_Buffer();
    textSourceEditor_ = new Fl_Text_Editor(x, y, contentWidth, editorHeight);
    textSourceEditor_->buffer(textSourceBuffer_);
    y += editorHeight + 10;
    new Fl_Box(x, y, 80, RowHeight, "Résultat");
    y += RowHeight;
    textResultBuffer_ = new Fl_Text_Buffer();
    textResultDisplay_ = new Fl_Text_Display(x, y, contentWidth, editorHeight);
    textResultDisplay_->buffer(textResultBuffer_);
    textTab_->end();

    filesTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Fichiers");
    filesTab_->begin();
    x = filesTab_->x() + Margin;
    y = filesTab_->y() + Margin;
    contentWidth = filesTab_->w() - Margin * 2;
    fileOperationChoice_ = new Fl_Choice(x + LabelWidth, y, 220, RowHeight, "Opération");
    fileOperationChoice_->add("Chiffrer");
    fileOperationChoice_->add("Déchiffrer");
    fileOperationChoice_->add("Signer détaché");
    fileOperationChoice_->add("Vérifier signature détachée");
    fileOperationChoice_->add("Vérifier fichier signé");
    fileOperationChoice_->value(0);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Source");
    fileSourceInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 110, RowHeight);
    auto chooseSourceButton = new Fl_Button(fileSourceInput_->x() + fileSourceInput_->w() + 8, y, 100, RowHeight, "Choisir");
    chooseSourceButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseFileSource(); }, this);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Destination");
    fileDestinationInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 110, RowHeight);
    auto chooseDestinationButton = new Fl_Button(fileDestinationInput_->x() + fileDestinationInput_->w() + 8, y, 100, RowHeight, "Choisir");
    chooseDestinationButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseFileDestination(); }, this);
    y += RowHeight + 10;
    new Fl_Box(x, y, LabelWidth, RowHeight, "Signature");
    fileSignatureInput_ = new Fl_Input(x + LabelWidth, y, contentWidth - LabelWidth - 220, RowHeight);
    auto chooseSignatureOpenButton = new Fl_Button(fileSignatureInput_->x() + fileSignatureInput_->w() + 8, y, 100, RowHeight, "Ouvrir");
    chooseSignatureOpenButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseSignatureFile(false); }, this);
    auto chooseSignatureSaveButton = new Fl_Button(chooseSignatureOpenButton->x() + chooseSignatureOpenButton->w() + 8, y, 100, RowHeight, "Enregistrer");
    chooseSignatureSaveButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->chooseSignatureFile(true); }, this);
    y += RowHeight + 14;
    auto executeFileButton = new Fl_Button(x + LabelWidth, y, 130, RowHeight, "Exécuter");
    executeFileButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->executeFileOperation(); }, this);
    fileStatusOutput_ = new Fl_Output(executeFileButton->x() + executeFileButton->w() + 12, y, contentWidth - LabelWidth - executeFileButton->w() - 12, RowHeight);
    filesTab_->end();

    logTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Journal");
    logTab_->begin();
    auto clearLogButton = new Fl_Button(logTab_->x() + Margin, logTab_->y() + Margin, 130, RowHeight, "Effacer journal");
    clearLogButton->callback([](Fl_Widget*, void* data) { static_cast<MainWindow*>(data)->clearLog(); }, this);
    logBuffer_ = new Fl_Text_Buffer();
    logDisplay_ = new Fl_Text_Display(logTab_->x() + Margin, logTab_->y() + Margin + RowHeight + 8, logTab_->w() - Margin * 2, logTab_->h() - Margin * 2 - RowHeight - 8);
    logDisplay_->buffer(logBuffer_);
    logTab_->end();

    tabs_->end();
    resizable(tabs_);
    end();
}

void MainWindow::loadInitialState() {
    if (settings_.window.width > 0 && settings_.window.height > 0) {
        size(settings_.window.width, settings_.window.height);
    }
    if (windowLooksVisible(settings_.window.x, settings_.window.y, settings_.window.width, settings_.window.height)) {
        position(settings_.window.x, settings_.window.y);
    }
    if (settings_.window.lastTab == "keys") {
        tabs_->value(keysTab_);
    } else if (settings_.window.lastTab == "send") {
        tabs_->value(sendTab_);
    } else if (settings_.window.lastTab == "receive") {
        tabs_->value(receiveTab_);
    } else if (settings_.window.lastTab == "package") {
        tabs_->value(packageTab_);
    } else if (settings_.window.lastTab == "text") {
        tabs_->value(textTab_);
    } else if (settings_.window.lastTab == "files") {
        tabs_->value(filesTab_);
    } else if (settings_.window.lastTab == "log") {
        tabs_->value(logTab_);
    } else {
        tabs_->value(homeTab_);
    }
    updateGpgPath(settings_.gpg.executablePath, false);
    if (!settings_.gpg.executablePath.empty()) {
        testGpg();
        reloadKeys();
    } else {
        updateStatus("Aucun exécutable gpg sélectionné.");
    }
    updateHomeStatus();
}

void MainWindow::saveSettings() {
    settings_.window.x = x();
    settings_.window.y = y();
    settings_.window.width = w();
    settings_.window.height = h();
    settings_.window.state = "normal";
    updateLastTab();
    settingsService_.save(settings_);
}

void MainWindow::updateHomeStatus() {
    if (!homeStatusOutput_) {
        return;
    }
    std::ostringstream status;
    status << (settings_.gpg.executablePath.empty() ? "GPG non configuré" : "GPG configuré");
    status << "  -  " << encryptionKeys_.size() << " destinataire(s)";
    status << "  -  " << signingKeys_.size() << " identité(s) locale(s)";
    if (!settings_.paths.lastOpenDir.empty()) {
        status << "  -  Dernier dossier : " << settings_.paths.lastOpenDir;
    }
    homeStatusOutput_->value(status.str().c_str());
}

void MainWindow::updateGpgPath(const std::string& path, bool saveNow) {
    settings_.gpg.executablePath = path;
    gpgPathInput_->value(path.c_str());
    if (saveNow) {
        saveSettings();
    }
}

void MainWindow::chooseGpgExecutable() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir l'exécutable gpg");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        updateGpgPath(chooser.filename(), true);
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        saveSettings();
        testGpg();
    }
}

void MainWindow::autoDetectGpg() {
    auto detected = GpgExecutable::detect();
    if (detected.empty()) {
        updateStatus("Aucun exécutable gpg valide détecté.");
        appendLog("Détection automatique de gpg échouée.");
        return;
    }
    updateGpgPath(detected, true);
    updateStatus("Exécutable gpg détecté.");
    appendLog("gpg détecté: " + detected);
    testGpg();
}

void MainWindow::testGpg() {
    updateGpgPath(gpgPathInput_->value() ? gpgPathInput_->value() : "", false);
    auto test = GpgExecutable(settings_.gpg.executablePath).test();
    if (test.valid) {
        std::istringstream lines(test.versionText);
        std::string firstLine;
        std::getline(lines, firstLine);
        updateStatus(firstLine.empty() ? "gpg valide." : firstLine);
        appendLog("Test gpg réussi.");
    } else {
        auto message = test.errorText.empty() ? "Exécutable gpg invalide." : test.errorText;
        updateStatus(message);
        appendLog("Test gpg échoué: " + message);
    }
    saveSettings();
}

void MainWindow::reloadKeys() {
    updateGpgPath(gpgPathInput_->value() ? gpgPathInput_->value() : "", true);
    if (settings_.gpg.executablePath.empty()) {
        updateStatus("Sélectionnez un exécutable gpg avant de charger les clés.");
        return;
    }
    std::string errorText;
    auto keys = KeyStore(settings_.gpg.executablePath).listMergedKeys(&errorText);
    encryptionKeys_.clear();
    signingKeys_.clear();
    for (const auto& key : keys) {
        if (key.canEncrypt) {
            encryptionKeys_.push_back(key);
        }
        if (key.hasSecretKey && key.canSign) {
            signingKeys_.push_back(key);
        }
    }
    populateKeyBrowsers();
    restoreKeySelections();
    if (!errorText.empty()) {
        updateStatus("Erreur lors du chargement des clés.");
        appendLog("Chargement des clés échoué: " + errorText);
        return;
    }
    std::ostringstream message;
    message << encryptionKeys_.size() << " clé(s) de chiffrement, " << signingKeys_.size() << " clé(s) de signature.";
    updateStatus(message.str());
    appendLog("Clés rechargées: " + message.str());
    updateHomeStatus();
}

void MainWindow::exportEncryptionPublicKey() {
    auto fingerprint = selectedEncryptionFingerprint();
    if (fingerprint.empty()) {
        fl_alert("Aucune clé de chiffrement sélectionnée.");
        return;
    }
    Fl_Native_File_Chooser chooser;
    chooser.title("Exporter la clé publique");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.filter("Clés ASCII armurées\t*.asc\nTexte\t*.txt\nTous les fichiers\t*");
    std::string defaultName = fingerprint + ".asc";
    chooser.preset_file(defaultName.c_str());
    if (directoryExists(settings_.paths.lastKeyExportDir)) {
        chooser.directory(settings_.paths.lastKeyExportDir.c_str());
    } else if (directoryExists(settings_.paths.lastSaveDir)) {
        chooser.directory(settings_.paths.lastSaveDir.c_str());
    }
    if (chooser.show() != 0 || !chooser.filename()) {
        return;
    }

    std::string errorText;
    auto exported = KeyStore(settings_.gpg.executablePath).exportPublicKey(fingerprint, &errorText);
    if (exported.empty()) {
        auto message = errorText.empty() ? "Export impossible." : errorText;
        fl_alert("%s", message.c_str());
        appendLog("Export de clé publique échoué: " + message);
        return;
    }

    std::ofstream output(chooser.filename(), std::ios::binary | std::ios::trunc);
    if (!output) {
        fl_alert("Impossible d'écrire le fichier de destination.");
        appendLog("Export de clé publique échoué: impossible d'écrire le fichier.");
        return;
    }
    output << exported;
    if (!output) {
        fl_alert("Erreur pendant l'écriture du fichier.");
        appendLog("Export de clé publique échoué pendant l'écriture.");
        return;
    }
    settings_.paths.lastKeyExportDir = directoryOf(chooser.filename());
    settings_.paths.lastSaveDir = settings_.paths.lastKeyExportDir;
    saveSettings();
    fl_message("Clé publique exportée.");
    appendLog("Clé publique exportée pour " + fingerprint);
}

void MainWindow::executeTextOperation() {
    updateGpgPath(gpgPathInput_->value() ? gpgPathInput_->value() : "", true);
    if (settings_.gpg.executablePath.empty()) {
        fl_alert("Sélectionnez un exécutable gpg avant d'exécuter une opération texte.");
        return;
    }

    std::string source = bufferText(textSourceBuffer_);
    if (source.empty()) {
        fl_alert("La zone source est vide.");
        return;
    }

    const int operation = textOperationChoice_->value();
    GpgProcessResult result;
    std::string operationName;
    if (operation == 0) {
        auto* key = selectedEncryptionKey();
        if (!key) {
            fl_alert("Sélectionnez une clé de chiffrement.");
            return;
        }
        if (!confirmKeyWarnings(*key, "chiffrer ce texte")) {
            return;
        }
        operationName = "Chiffrement texte";
        result = CryptoService(settings_.gpg.executablePath).encryptText(source, key->fingerprint);
    } else if (operation == 1) {
        operationName = "Déchiffrement texte";
        result = CryptoService(settings_.gpg.executablePath).decryptText(source);
    } else if (operation == 2) {
        auto* key = selectedSigningKey();
        if (!key) {
            fl_alert("Sélectionnez une clé de signature.");
            return;
        }
        if (!confirmKeyWarnings(*key, "signer ce texte")) {
            return;
        }
        operationName = "Signature texte";
        result = SignatureService(settings_.gpg.executablePath).signText(source, key->fingerprint);
    } else {
        operationName = "Vérification texte";
        result = SignatureService(settings_.gpg.executablePath).verifyText(source);
    }

    if (operation == 3) {
        auto summary = SignatureService::summarizeVerification(result);
        textResultBuffer_->text(summary.message.c_str());
    } else if (result.success()) {
        textResultBuffer_->text(result.standardOutput.c_str());
    } else {
        auto diagnostic = diagnosticText(result);
        textResultBuffer_->text(diagnostic.empty() ? "L'opération GPG a échoué." : diagnostic.c_str());
    }

    if (result.success()) {
        appendLog(operationName + " réussi.");
    } else {
        std::ostringstream message;
        message << operationName << " échoué";
        if (result.exitCode >= 0) {
            message << " (code " << result.exitCode << ")";
        }
        message << ".";
        appendLog(message.str());
    }
}

void MainWindow::copyTextResult() {
    auto result = bufferText(textResultBuffer_);
    if (result.empty()) {
        fl_alert("Aucun résultat à copier.");
        return;
    }
    Fl::copy(result.c_str(), static_cast<int>(result.size()), 1);
    appendLog("Résultat texte copié dans le presse-papiers.");
}

void MainWindow::clearClipboard() {
    Fl::copy("", 0, 1);
    appendLog("Presse-papiers vidé à la demande.");
}

void MainWindow::clearTextBuffers() {
    textSourceBuffer_->text("");
    textResultBuffer_->text("");
    appendLog("Zones texte effacées.");
}

void MainWindow::saveTextResult() {
    auto result = bufferText(textResultBuffer_);
    if (result.empty()) {
        fl_alert("Aucun résultat à enregistrer.");
        return;
    }

    Fl_Native_File_Chooser chooser;
    chooser.title("Enregistrer le résultat");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.filter("Texte\t*.txt\nASCII armuré\t*.asc\nTous les fichiers\t*");
    chooser.preset_file("sealkey-result.txt");
    if (directoryExists(settings_.paths.lastSaveDir)) {
        chooser.directory(settings_.paths.lastSaveDir.c_str());
    }
    if (chooser.show() != 0 || !chooser.filename()) {
        return;
    }

    std::ofstream output(chooser.filename(), std::ios::binary | std::ios::trunc);
    if (!output) {
        fl_alert("Impossible d'écrire le fichier de destination.");
        appendLog("Enregistrement du résultat texte échoué.");
        return;
    }
    output << result;
    if (!output) {
        fl_alert("Erreur pendant l'écriture du fichier.");
        appendLog("Enregistrement du résultat texte échoué pendant l'écriture.");
        return;
    }
    settings_.paths.lastSaveDir = directoryOf(chooser.filename());
    saveSettings();
    appendLog("Résultat texte enregistré.");
}

void MainWindow::chooseFileSource() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir le fichier source");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        fileSourceInput_->value(chooser.filename());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        saveSettings();
    }
}

void MainWindow::chooseFileDestination() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir le fichier destination");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    auto defaultName = defaultFileNameForOperation(fileSourceInput_->value() ? fileSourceInput_->value() : "",
                                                   fileOperationChoice_->value());
    chooser.preset_file(defaultName.c_str());
    if (directoryExists(settings_.paths.lastSaveDir)) {
        chooser.directory(settings_.paths.lastSaveDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        fileDestinationInput_->value(chooser.filename());
        settings_.paths.lastSaveDir = directoryOf(chooser.filename());
        saveSettings();
    }
}

void MainWindow::chooseSignatureFile(bool saveMode) {
    Fl_Native_File_Chooser chooser;
    chooser.title(saveMode ? "Choisir le fichier signature à écrire" : "Choisir le fichier signature");
    chooser.type(saveMode ? Fl_Native_File_Chooser::BROWSE_SAVE_FILE : Fl_Native_File_Chooser::BROWSE_FILE);
    chooser.filter("Signatures ASCII\t*.asc\nSignatures\t*.sig\nTous les fichiers\t*");
    if (saveMode) {
        auto defaultName = defaultFileNameForOperation(fileSourceInput_->value() ? fileSourceInput_->value() : "", 2);
        chooser.preset_file(defaultName.c_str());
        if (directoryExists(settings_.paths.lastSignatureSaveDir)) {
            chooser.directory(settings_.paths.lastSignatureSaveDir.c_str());
        } else if (directoryExists(settings_.paths.lastSaveDir)) {
            chooser.directory(settings_.paths.lastSaveDir.c_str());
        }
    } else if (directoryExists(settings_.paths.lastSignatureOpenDir)) {
        chooser.directory(settings_.paths.lastSignatureOpenDir.c_str());
    } else if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        fileSignatureInput_->value(chooser.filename());
        if (saveMode) {
            settings_.paths.lastSignatureSaveDir = directoryOf(chooser.filename());
            settings_.paths.lastSaveDir = settings_.paths.lastSignatureSaveDir;
        } else {
            settings_.paths.lastSignatureOpenDir = directoryOf(chooser.filename());
            settings_.paths.lastOpenDir = settings_.paths.lastSignatureOpenDir;
        }
        saveSettings();
    }
}

void MainWindow::executeFileOperation() {
    updateGpgPath(gpgPathInput_->value() ? gpgPathInput_->value() : "", true);
    if (settings_.gpg.executablePath.empty()) {
        fl_alert("Sélectionnez un exécutable gpg avant d'exécuter une opération fichier.");
        return;
    }

    const int operation = fileOperationChoice_->value();
    std::string source = fileSourceInput_->value() ? fileSourceInput_->value() : "";
    std::string destination = fileDestinationInput_->value() ? fileDestinationInput_->value() : "";
    std::string signature = fileSignatureInput_->value() ? fileSignatureInput_->value() : "";

    if (!fileExists(source)) {
        fl_alert("Sélectionnez un fichier source existant.");
        return;
    }
    if ((operation == 0 || operation == 1) && destination.empty()) {
        fl_alert("Sélectionnez un fichier destination.");
        return;
    }
    if (operation == 2 && signature.empty()) {
        fl_alert("Sélectionnez le fichier signature à écrire.");
        return;
    }
    if (operation == 3 && !fileExists(signature)) {
        fl_alert("Sélectionnez un fichier signature existant.");
        return;
    }

    GpgProcessResult result;
    std::string operationName;
    if (operation == 0) {
        auto* key = selectedEncryptionKey();
        if (!key) {
            fl_alert("Sélectionnez une clé de chiffrement.");
            return;
        }
        if (!confirmKeyWarnings(*key, "chiffrer ce fichier")) {
            return;
        }
        operationName = "Chiffrement fichier";
        result = CryptoService(settings_.gpg.executablePath).encryptFile(source, destination, key->fingerprint, true);
    } else if (operation == 1) {
        operationName = "Déchiffrement fichier";
        result = CryptoService(settings_.gpg.executablePath).decryptFile(source, destination);
    } else if (operation == 2) {
        auto* key = selectedSigningKey();
        if (!key) {
            fl_alert("Sélectionnez une clé de signature.");
            return;
        }
        if (!confirmKeyWarnings(*key, "signer ce fichier")) {
            return;
        }
        operationName = "Signature fichier";
        result = SignatureService(settings_.gpg.executablePath).signFileDetached(source, signature, key->fingerprint);
    } else if (operation == 3) {
        operationName = "Vérification signature détachée";
        result = SignatureService(settings_.gpg.executablePath).verifyDetachedFile(signature, source);
    } else {
        operationName = "Vérification fichier signé";
        result = SignatureService(settings_.gpg.executablePath).verifySignedFile(source);
    }

    if (result.success()) {
        if (operation == 3 || operation == 4) {
            auto summary = SignatureService::summarizeVerification(result);
            fileStatusOutput_->value(summary.message.c_str());
        } else {
            fileStatusOutput_->value("Opération réussie.");
        }
        settings_.paths.lastOpenDir = directoryOf(source);
        if (!destination.empty()) {
            settings_.paths.lastSaveDir = directoryOf(destination);
        }
        if (!signature.empty()) {
            if (operation == 2) {
                settings_.paths.lastSignatureSaveDir = directoryOf(signature);
            } else {
                settings_.paths.lastSignatureOpenDir = directoryOf(signature);
            }
        }
        saveSettings();
        appendLog(operationName + " réussi.");
        return;
    }

    auto diagnostic = diagnosticText(result);
    if (operation == 3 || operation == 4) {
        auto summary = SignatureService::summarizeVerification(result);
        fileStatusOutput_->value(summary.message.c_str());
    } else {
        fileStatusOutput_->value(diagnostic.empty() ? "L'opération GPG a échoué." : diagnostic.c_str());
    }
    std::ostringstream message;
    message << operationName << " échoué";
    if (result.exitCode >= 0) {
        message << " (code " << result.exitCode << ")";
    }
    message << ".";
    appendLog(message.str());
}

void MainWindow::chooseSendSource() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Quel fichier voulez-vous envoyer ?");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        sendSourceInput_->value(chooser.filename());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        auto output = std::filesystem::path(settings_.paths.lastSaveDir.empty() ? settings_.paths.lastOpenDir : settings_.paths.lastSaveDir) /
                      defaultProtectedFileName(chooser.filename(), sendActionChoice_->value() != 1);
        sendOutputInput_->value(output.string().c_str());
        saveSettings();
        updateHomeStatus();
    }
}

void MainWindow::chooseSendOutput() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Où créer le fichier à envoyer ?");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.preset_file(defaultProtectedFileName(sendSourceInput_->value() ? sendSourceInput_->value() : "",
                                                 sendActionChoice_->value() != 1).c_str());
    if (directoryExists(settings_.paths.lastSaveDir)) {
        chooser.directory(settings_.paths.lastSaveDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        sendOutputInput_->value(chooser.filename());
        settings_.paths.lastSaveDir = directoryOf(chooser.filename());
        saveSettings();
    }
}

void MainWindow::executeSendOperation() {
    const std::string source = sendSourceInput_->value() ? sendSourceInput_->value() : "";
    const std::string output = sendOutputInput_->value() ? sendOutputInput_->value() : "";
    const int action = sendActionChoice_->value();
    if (!fileExists(source)) {
        fl_alert("Sélectionnez le fichier à envoyer.");
        return;
    }
    if (output.empty()) {
        fl_alert("Choisissez le fichier de sortie.");
        return;
    }
    const GpgKey* recipient = nullptr;
    if (action == 0 || action == 2) {
        int index = sendRecipientBrowser_->value();
        if (index <= 0 || index > static_cast<int>(encryptionKeys_.size())) {
            fl_alert("Choisissez le destinataire.");
            return;
        }
        recipient = &encryptionKeys_[index - 1];
        if (!confirmKeyWarnings(*recipient, "envoyer ce fichier à ce destinataire")) {
            return;
        }
    }
    const GpgKey* signer = nullptr;
    if (action == 1 || action == 2) {
        signer = selectedSigningKey();
        if (!signer) {
            fl_alert("Choisissez une clé de signature dans l'onglet Clés.");
            return;
        }
        if (!confirmKeyWarnings(*signer, "signer ce fichier")) {
            return;
        }
    }

    std::ostringstream summary;
    summary << "Résumé\n\nFichier source :\n" << source << "\n\nAction :\n";
    summary << (action == 0 ? "Chiffrer" : action == 1 ? "Signer" : "Chiffrer et signer");
    if (recipient) {
        summary << "\n\nDestinataire :\n" << keyLabel(*recipient);
    }
    if (signer) {
        summary << "\n\nSignature :\n" << keyLabel(*signer);
    }
    summary << "\n\nFichier produit :\n" << output;
    if (fl_choice("%s", "Annuler", "Exécuter", "", summary.str().c_str()) != 1) {
        return;
    }

    GpgProcessResult process;
    if (action == 0) {
        process = CryptoService(settings_.gpg.executablePath).encryptFile(source, output, recipient->fingerprint, true);
    } else if (action == 1) {
        process = SignatureService(settings_.gpg.executablePath).signFileDetached(source, output, signer->fingerprint);
    } else {
        process = CryptoService(settings_.gpg.executablePath).encryptAndSignFile(source, output, recipient->fingerprint, signer->fingerprint, true);
    }

    OperationResult result;
    result.success = process.success();
    result.technicalMessage = diagnosticText(process);
    if (result.success) {
        result.userMessage = action == 0
                                 ? "Le fichier a été chiffré pour le destinataire. Seul ce destinataire pourra le déchiffrer avec sa clé privée."
                                 : action == 1
                                       ? "Signature créée. Envoyez le fichier original et le fichier de signature au destinataire."
                                       : "Le fichier est chiffré pour le destinataire et signé avec votre identité.";
        result.outputFiles.push_back(output);
        result.suggestedNextActions.push_back("Envoyer le fichier produit au destinataire.");
        settings_.paths.lastOpenDir = directoryOf(source);
        settings_.paths.lastSaveDir = directoryOf(output);
        saveSettings();
        appendLog("Parcours Envoyer réussi.");
    } else {
        result.userMessage = readableGpgError(process);
        result.suggestedNextActions.push_back("Vérifier la configuration GPG.");
        result.suggestedNextActions.push_back("Vérifier les clés sélectionnées.");
        appendLog("Parcours Envoyer échoué.");
    }
    setOperationResult(sendResultBuffer_, result);
}

void MainWindow::chooseReceiveFile() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Quel fichier avez-vous reçu ?");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        receiveFileInput_->value(chooser.filename());
        receiveDetectedOutput_->value(fileKindForPath(chooser.filename()).c_str());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        auto output = std::filesystem::path(settings_.paths.lastSaveDir.empty() ? settings_.paths.lastOpenDir : settings_.paths.lastSaveDir) /
                      defaultDecryptedFileName(chooser.filename());
        receiveOutputInput_->value(output.string().c_str());
        saveSettings();
        updateHomeStatus();
    }
}

void MainWindow::chooseReceiveOutput() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Où écrire le fichier ouvert ?");
    chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.preset_file(defaultDecryptedFileName(receiveFileInput_->value() ? receiveFileInput_->value() : "").c_str());
    if (directoryExists(settings_.paths.lastSaveDir)) {
        chooser.directory(settings_.paths.lastSaveDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        receiveOutputInput_->value(chooser.filename());
        settings_.paths.lastSaveDir = directoryOf(chooser.filename());
        saveSettings();
    }
}

void MainWindow::chooseReceiveOriginal() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Fichier original correspondant à la signature");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        receiveOriginalInput_->value(chooser.filename());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        saveSettings();
    }
}

void MainWindow::executeReceiveOperation() {
    const std::string file = receiveFileInput_->value() ? receiveFileInput_->value() : "";
    const std::string output = receiveOutputInput_->value() ? receiveOutputInput_->value() : "";
    const std::string original = receiveOriginalInput_->value() ? receiveOriginalInput_->value() : "";
    if (!fileExists(file)) {
        fl_alert("Sélectionnez le fichier reçu.");
        return;
    }

    OperationResult result;
    GpgProcessResult process;
    const bool looksEncrypted = fileKindForPath(file).find("chiffré") != std::string::npos ||
                                fileContainsMarker(file, "BEGIN PGP MESSAGE");
    const bool looksDetachedSignature = std::filesystem::path(file).extension() == ".sig";
    const bool looksPublicKey = fileContainsMarker(file, "BEGIN PGP PUBLIC KEY BLOCK");
    const bool looksSignedText = fileContainsMarker(file, "BEGIN PGP SIGNED MESSAGE") ||
                                 fileContainsMarker(file, "BEGIN PGP SIGNATURE");

    if (looksPublicKey) {
        if (fl_choice("Ce fichier ressemble à une clé publique.\nVoulez-vous l'importer ?", "Annuler", "Importer", "") != 1) {
            return;
        }
        process = KeyStore(settings_.gpg.executablePath).importPublicKey(file);
        result.success = process.success();
        result.userMessage = result.success ? "Clé publique importée. Vous pouvez maintenant vérifier des signatures ou chiffrer pour ce correspondant."
                                            : readableGpgError(process);
        result.technicalMessage = diagnosticText(process);
        if (result.success) {
            reloadKeys();
        }
    } else if (looksEncrypted) {
        if (output.empty()) {
            fl_alert("Choisissez le fichier de sortie pour le déchiffrement.");
            return;
        }
        process = CryptoService(settings_.gpg.executablePath).decryptFile(file, output);
        result.success = process.success();
        result.userMessage = result.success ? "Le fichier a été déchiffré avec succès."
                                            : "Ce fichier ne peut pas être déchiffré avec vos clés privées. Il n'a probablement pas été chiffré pour vous, ou vous n'utilisez pas le bon trousseau GPG.";
        result.technicalMessage = diagnosticText(process);
        if (result.success) {
            result.outputFiles.push_back(output);
            settings_.paths.lastSaveDir = directoryOf(output);
        }
    } else if (looksDetachedSignature) {
        if (!fileExists(original)) {
            fl_alert("Une signature détachée nécessite le fichier original.");
            return;
        }
        process = SignatureService(settings_.gpg.executablePath).verifyDetachedFile(file, original);
        auto summary = SignatureService::summarizeVerification(process);
        result.success = summary.valid;
        result.userMessage = summary.valid
                                 ? "Signature valide. Le fichier correspond bien à cette signature."
                                 : readableGpgError(process);
        if (summary.unknownKey) {
            result.warnings.push_back("La clé publique de l'expéditeur n'est pas connue.");
        }
        if (summary.expiredKey) {
            result.warnings.push_back("La clé ou la signature est expirée.");
        }
        if (summary.revokedKey) {
            result.warnings.push_back("La clé du signataire est révoquée.");
        }
        if (summary.untrustedKey) {
            result.warnings.push_back("La signature est cryptographiquement valide, mais la confiance n'est pas établie.");
        }
        result.technicalMessage = diagnosticText(process);
    } else if (looksSignedText) {
        process = SignatureService(settings_.gpg.executablePath).verifySignedFile(file);
        auto summary = SignatureService::summarizeVerification(process);
        result.success = summary.valid;
        result.userMessage = summary.valid
                                 ? "Signature valide. Le fichier signé n'a pas été modifié depuis sa signature."
                                 : readableGpgError(process);
        if (summary.unknownKey) {
            result.warnings.push_back("La clé publique de l'expéditeur n'est pas connue.");
        }
        if (summary.expiredKey) {
            result.warnings.push_back("La clé ou la signature est expirée.");
        }
        if (summary.revokedKey) {
            result.warnings.push_back("La clé du signataire est révoquée.");
        }
        if (summary.untrustedKey) {
            result.warnings.push_back("La signature est cryptographiquement valide, mais la confiance n'est pas établie.");
        }
        result.technicalMessage = diagnosticText(process);
    } else {
        result.userMessage = "SealKey n'a pas reconnu automatiquement ce fichier. Essayez l'onglet Fichiers avancé pour choisir l'opération explicitement.";
        result.suggestedNextActions.push_back("Utiliser l'onglet Fichiers.");
    }

    settings_.paths.lastOpenDir = directoryOf(file);
    saveSettings();
    setOperationResult(receiveResultBuffer_, result);
    appendLog(result.success ? "Parcours Recevoir réussi." : "Parcours Recevoir terminé avec avertissement ou erreur.");
}

void MainWindow::addPackageFile() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Ajouter un fichier au dossier d'envoi");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        packageFiles_.push_back(chooser.filename());
        packageFilesBrowser_->add(std::filesystem::path(chooser.filename()).filename().string().c_str());
        settings_.paths.lastOpenDir = directoryOf(chooser.filename());
        if (packageOutputDirInput_->value() == nullptr || std::string(packageOutputDirInput_->value()).empty()) {
            auto defaultDir = std::filesystem::path(settings_.paths.lastSaveDir.empty() ? settings_.paths.lastOpenDir : settings_.paths.lastSaveDir) /
                              makePackageDirectoryName();
            packageOutputDirInput_->value(defaultDir.string().c_str());
        }
        saveSettings();
    }
}

void MainWindow::clearPackageFiles() {
    packageFiles_.clear();
    packageFilesBrowser_->clear();
    packageResultBuffer_->text("");
}

void MainWindow::choosePackageOutputDir() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Choisir le dossier de sortie");
    chooser.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
    if (directoryExists(settings_.paths.lastSaveDir)) {
        chooser.directory(settings_.paths.lastSaveDir.c_str());
    }
    if (chooser.show() == 0 && chooser.filename()) {
        auto outputDir = std::filesystem::path(chooser.filename()) / makePackageDirectoryName();
        packageOutputDirInput_->value(outputDir.string().c_str());
        settings_.paths.lastSaveDir = chooser.filename();
        saveSettings();
    }
}

void MainWindow::executePackageOperation() {
    if (packageFiles_.empty()) {
        fl_alert("Ajoutez au moins un fichier.");
        return;
    }
    const std::string outputDirText = packageOutputDirInput_->value() ? packageOutputDirInput_->value() : "";
    if (outputDirText.empty()) {
        fl_alert("Choisissez un dossier de sortie.");
        return;
    }
    const int action = packageActionChoice_->value();
    const GpgKey* recipient = nullptr;
    if (action == 1 || action == 2) {
        int index = packageRecipientBrowser_->value();
        if (index <= 0 || index > static_cast<int>(encryptionKeys_.size())) {
            fl_alert("Choisissez le destinataire.");
            return;
        }
        recipient = &encryptionKeys_[index - 1];
        if (!confirmKeyWarnings(*recipient, "préparer ce dossier d'envoi")) {
            return;
        }
    }
    const GpgKey* signer = nullptr;
    if (action == 0 || action == 2 || packageIncludePublicKeyCheck_->value()) {
        signer = selectedSigningKey();
        if (!signer) {
            fl_alert("Choisissez une clé de signature dans l'onglet Clés.");
            return;
        }
        if ((action == 0 || action == 2) && !confirmKeyWarnings(*signer, "signer les fichiers du dossier")) {
            return;
        }
    }

    std::ostringstream summary;
    summary << "Résumé\n\nFichiers : " << packageFiles_.size() << "\nAction : "
            << (action == 0 ? "Signer" : action == 1 ? "Chiffrer" : "Chiffrer et signer")
            << "\nDossier : " << outputDirText;
    if (recipient) {
        summary << "\nDestinataire : " << keyLabel(*recipient);
    }
    if (signer) {
        summary << "\nSignature : " << keyLabel(*signer);
    }
    if (packageIncludePublicKeyCheck_->value()) {
        summary << "\nLa clé publique de signature sera jointe.";
    }
    if (fl_choice("%s", "Annuler", "Créer", "", summary.str().c_str()) != 1) {
        return;
    }

    OperationResult result;
    std::filesystem::path outputDir(outputDirText);
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    if (ec) {
        result.userMessage = "Impossible de créer le dossier de sortie.";
        result.technicalMessage = ec.message();
        setOperationResult(packageResultBuffer_, result);
        return;
    }

    bool allOk = true;
    std::ostringstream technical;
    for (const auto& inputFile : packageFiles_) {
        auto sourcePath = std::filesystem::path(inputFile);
        auto outputPath = outputDir / defaultProtectedFileName(inputFile, action != 0);
        GpgProcessResult process;
        if (action == 0) {
            outputPath = outputDir / (sourcePath.filename().string() + ".sig.asc");
            process = SignatureService(settings_.gpg.executablePath).signFileDetached(inputFile, outputPath.string(), signer->fingerprint);
        } else if (action == 1) {
            process = CryptoService(settings_.gpg.executablePath).encryptFile(inputFile, outputPath.string(), recipient->fingerprint, true);
        } else {
            process = CryptoService(settings_.gpg.executablePath).encryptAndSignFile(inputFile, outputPath.string(), recipient->fingerprint, signer->fingerprint, true);
        }
        technical << "\n[" << sourcePath.filename().string() << "]\n" << diagnosticText(process) << "\n";
        if (process.success()) {
            result.outputFiles.push_back(outputPath.string());
        } else {
            allOk = false;
            result.warnings.push_back(sourcePath.filename().string() + " : " + readableGpgError(process));
        }
    }

    if (packageIncludePublicKeyCheck_->value() && signer) {
        std::string errorText;
        auto exported = KeyStore(settings_.gpg.executablePath).exportPublicKey(signer->fingerprint, &errorText);
        if (!exported.empty()) {
            auto keyPath = outputDir / (displayNameFromUid(signer->uid) + "_" + shortFingerprint(signer->fingerprint) + "_GPG_PUB.asc");
            std::ofstream keyFile(keyPath, std::ios::binary | std::ios::trunc);
            keyFile << exported;
            if (keyFile) {
                result.outputFiles.push_back(keyPath.string());
            }
        } else {
            result.warnings.push_back("La clé publique n'a pas pu être ajoutée : " + errorText);
        }
    }

    auto readmePath = outputDir / "README.txt";
    std::ofstream readme(readmePath, std::ios::trunc);
    readme << "Ce dossier contient des fichiers préparés avec SealKey.\n\n";
    readme << "Action : " << (action == 0 ? "signature" : action == 1 ? "chiffrement" : "chiffrement et signature") << "\n";
    if (recipient) {
        readme << "Destinataire : " << recipient->uid << "\n";
    }
    if (signer) {
        readme << "Signé par : " << signer->uid << "\n";
    }
    readme << "\nFichiers produits :\n";
    for (const auto& file : result.outputFiles) {
        readme << "- " << std::filesystem::path(file).filename().string() << "\n";
    }
    if (readme) {
        result.outputFiles.push_back(readmePath.string());
    }

    result.success = allOk;
    result.userMessage = allOk ? "Dossier prêt à envoyer." : "Dossier créé, mais certaines opérations ont échoué.";
    result.technicalMessage = technical.str();
    result.suggestedNextActions.push_back("Envoyer le dossier complet au destinataire.");
    settings_.paths.lastSaveDir = outputDir.parent_path().string();
    saveSettings();
    setOperationResult(packageResultBuffer_, result);
    appendLog(allOk ? "Parcours Préparer réussi." : "Parcours Préparer terminé avec erreurs.");
}

void MainWindow::importPublicKey() {
    Fl_Native_File_Chooser chooser;
    chooser.title("Importer une clé publique");
    chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
    chooser.filter("Clés publiques\t*.asc\nTous les fichiers\t*");
    if (directoryExists(settings_.paths.lastOpenDir)) {
        chooser.directory(settings_.paths.lastOpenDir.c_str());
    }
    if (chooser.show() != 0 || !chooser.filename()) {
        return;
    }
    if (fl_choice("Importer cette clé publique dans le trousseau GPG ?", "Annuler", "Importer", "") != 1) {
        return;
    }
    auto process = KeyStore(settings_.gpg.executablePath).importPublicKey(chooser.filename());
    settings_.paths.lastOpenDir = directoryOf(chooser.filename());
    saveSettings();
    if (process.success()) {
        fl_message("Clé publique importée.");
        appendLog("Clé publique importée.");
        reloadKeys();
    } else {
        auto message = readableGpgError(process);
        fl_alert("%s", message.c_str());
        appendLog("Import de clé publique échoué.");
    }
}

void MainWindow::goToTab(Fl_Group* tab) {
    if (tab) {
        tabs_->value(tab);
        updateLastTab();
        saveSettings();
    }
}

void MainWindow::populateKeyBrowsers() {
    encryptionBrowser_->clear();
    signingBrowser_->clear();
    if (sendRecipientBrowser_) {
        sendRecipientBrowser_->clear();
    }
    if (packageRecipientBrowser_) {
        packageRecipientBrowser_->clear();
    }
    for (const auto& key : encryptionKeys_) {
        encryptionBrowser_->add(keyLabel(key).c_str());
        if (sendRecipientBrowser_) {
            auto contact = contactFromKey(key);
            std::ostringstream label;
            label << (contact.name.empty() ? key.uid : contact.name);
            if (!contact.email.empty()) {
                label << " <" << contact.email << ">";
            }
            label << "  [" << shortFingerprint(contact.fingerprint) << "]";
            if (contact.trustLevel != "f" && contact.trustLevel != "u") {
                label << "  confiance inconnue";
            }
            sendRecipientBrowser_->add(label.str().c_str());
            if (packageRecipientBrowser_) {
                packageRecipientBrowser_->add(label.str().c_str());
            }
        }
    }
    for (const auto& key : signingKeys_) {
        signingBrowser_->add(keyLabel(key).c_str());
    }
}

void MainWindow::restoreKeySelections() {
    encryptionBrowser_->value(0);
    signingBrowser_->value(0);
    for (int i = 0; i < static_cast<int>(encryptionKeys_.size()); ++i) {
        if (encryptionKeys_[i].fingerprint == settings_.keys.encryptionFingerprint) {
            encryptionBrowser_->value(i + 1);
            break;
        }
    }
    for (int i = 0; i < static_cast<int>(signingKeys_.size()); ++i) {
        if (signingKeys_[i].fingerprint == settings_.keys.signingFingerprint) {
            signingBrowser_->value(i + 1);
            break;
        }
    }
    if (encryptionBrowser_->value() == 0) {
        settings_.keys.encryptionFingerprint.clear();
    }
    if (signingBrowser_->value() == 0) {
        settings_.keys.signingFingerprint.clear();
    }
    auto* encryptionKey = selectedEncryptionKey();
    auto* signingKey = selectedSigningKey();
    encryptionFingerprintOutput_->value(encryptionKey ? keyDetailText(*encryptionKey, "Aucune clé de chiffrement sélectionnée").c_str()
                                                       : "Aucune clé de chiffrement sélectionnée");
    signingFingerprintOutput_->value(signingKey ? keyDetailText(*signingKey, "Aucune clé de signature sélectionnée").c_str()
                                                : "Aucune clé de signature sélectionnée");
    saveSettings();
}

void MainWindow::appendLog(const std::string& message) {
    if (!logBuffer_) {
        return;
    }
    auto line = currentTimestamp() + "  " + message + "\n";
    logBuffer_->append(line.c_str());
}

void MainWindow::clearLog() {
    if (logBuffer_) {
        logBuffer_->text("");
    }
}

void MainWindow::updateStatus(const std::string& message) {
    gpgStatusOutput_->value(message.c_str());
}

std::string MainWindow::selectedEncryptionFingerprint() const {
    int index = encryptionBrowser_->value();
    if (index <= 0 || index > static_cast<int>(encryptionKeys_.size())) {
        return {};
    }
    return encryptionKeys_[index - 1].fingerprint;
}

std::string MainWindow::selectedSigningFingerprint() const {
    int index = signingBrowser_->value();
    if (index <= 0 || index > static_cast<int>(signingKeys_.size())) {
        return {};
    }
    return signingKeys_[index - 1].fingerprint;
}

const GpgKey* MainWindow::selectedEncryptionKey() const {
    int index = encryptionBrowser_->value();
    if (index <= 0 || index > static_cast<int>(encryptionKeys_.size())) {
        return nullptr;
    }
    return &encryptionKeys_[index - 1];
}

const GpgKey* MainWindow::selectedSigningKey() const {
    int index = signingBrowser_->value();
    if (index <= 0 || index > static_cast<int>(signingKeys_.size())) {
        return nullptr;
    }
    return &signingKeys_[index - 1];
}

bool MainWindow::confirmKeyWarnings(const GpgKey& key, const std::string& usage) {
    auto warnings = keyWarningText(key);
    if (warnings.empty()) {
        return true;
    }
    std::ostringstream message;
    message << warnings << "\nEmpreinte : " << key.fingerprint << "\n\nContinuer pour " << usage << " ?";
    return fl_choice("%s", "Annuler", "Continuer", "", message.str().c_str()) == 1;
}

void MainWindow::updateLastTab() {
    auto* selected = tabs_->value();
    if (selected == keysTab_) {
        settings_.window.lastTab = "keys";
    } else if (selected == homeTab_) {
        settings_.window.lastTab = "home";
    } else if (selected == sendTab_) {
        settings_.window.lastTab = "send";
    } else if (selected == receiveTab_) {
        settings_.window.lastTab = "receive";
    } else if (selected == packageTab_) {
        settings_.window.lastTab = "package";
    } else if (selected == logTab_) {
        settings_.window.lastTab = "log";
    } else if (selected == textTab_) {
        settings_.window.lastTab = "text";
    } else if (selected == filesTab_) {
        settings_.window.lastTab = "files";
    } else {
        settings_.window.lastTab = "configuration";
    }
}

void MainWindow::onClose(Fl_Widget* widget, void* data) {
    auto* window = static_cast<MainWindow*>(data);
    window->saveSettings();
    widget->hide();
}
