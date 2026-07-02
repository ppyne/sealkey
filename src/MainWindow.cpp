#include "MainWindow.hpp"

#include "AppInfo.hpp"
#include "CryptoService.hpp"
#include "GpgExecutable.hpp"
#include "KeyStore.hpp"
#include "SignatureService.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
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

    configurationTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Configuration");
    configurationTab_->begin();
    int x = configurationTab_->x() + Margin;
    int y = configurationTab_->y() + Margin;
    int contentWidth = configurationTab_->w() - Margin * 2;
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
    } else if (settings_.window.lastTab == "text") {
        tabs_->value(textTab_);
    } else if (settings_.window.lastTab == "files") {
        tabs_->value(filesTab_);
    } else if (settings_.window.lastTab == "log") {
        tabs_->value(logTab_);
    } else {
        tabs_->value(configurationTab_);
    }
    updateGpgPath(settings_.gpg.executablePath, false);
    if (!settings_.gpg.executablePath.empty()) {
        testGpg();
        reloadKeys();
    } else {
        updateStatus("Aucun exécutable gpg sélectionné.");
    }
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

void MainWindow::populateKeyBrowsers() {
    encryptionBrowser_->clear();
    signingBrowser_->clear();
    for (const auto& key : encryptionKeys_) {
        encryptionBrowser_->add(keyLabel(key).c_str());
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
