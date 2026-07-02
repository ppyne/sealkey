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
    return label.str();
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
        window->encryptionFingerprintOutput_->value(window->settings_.keys.encryptionFingerprint.c_str());
        window->saveSettings();
    }, this);
    signingBrowser_ = new Fl_Hold_Browser(x + browserWidth + 16, y, browserWidth, 300);
    signingBrowser_->callback([](Fl_Widget*, void* data) {
        auto* window = static_cast<MainWindow*>(data);
        window->settings_.keys.signingFingerprint = window->selectedSigningFingerprint();
        window->signingFingerprintOutput_->value(window->settings_.keys.signingFingerprint.c_str());
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
    auto clearTextButton = new Fl_Button(copyTextButton->x() + copyTextButton->w() + 8, y, 80, RowHeight, "Effacer");
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
    new Fl_Box(filesTab_->x() + Margin, filesTab_->y() + Margin, 380, RowHeight, "Fonctions fichiers prévues en version 0.3");
    filesTab_->deactivate();
    filesTab_->end();

    logTab_ = new Fl_Group(Margin + 4, Margin + 28, w() - Margin * 2 - 8, h() - Margin * 2 - 32, "Journal");
    logTab_->begin();
    logBuffer_ = new Fl_Text_Buffer();
    logDisplay_ = new Fl_Text_Display(logTab_->x() + Margin, logTab_->y() + Margin, logTab_->w() - Margin * 2, logTab_->h() - Margin * 2);
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
    if (settings_.window.x >= 0 && settings_.window.y >= 0) {
        position(settings_.window.x, settings_.window.y);
    }
    if (settings_.window.lastTab == "keys") {
        tabs_->value(keysTab_);
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
        if (!key.expired && !key.revoked && key.canEncrypt) {
            encryptionKeys_.push_back(key);
        }
        if (!key.expired && !key.revoked && key.hasSecretKey && key.canSign) {
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
        auto fingerprint = selectedEncryptionFingerprint();
        if (fingerprint.empty()) {
            fl_alert("Sélectionnez une clé de chiffrement.");
            return;
        }
        operationName = "Chiffrement texte";
        result = CryptoService(settings_.gpg.executablePath).encryptText(source, fingerprint);
    } else if (operation == 1) {
        operationName = "Déchiffrement texte";
        result = CryptoService(settings_.gpg.executablePath).decryptText(source);
    } else if (operation == 2) {
        auto fingerprint = selectedSigningFingerprint();
        if (fingerprint.empty()) {
            fl_alert("Sélectionnez une clé de signature.");
            return;
        }
        operationName = "Signature texte";
        result = SignatureService(settings_.gpg.executablePath).signText(source, fingerprint);
    } else {
        operationName = "Vérification texte";
        result = SignatureService(settings_.gpg.executablePath).verifyText(source);
    }

    if (operation == 3) {
        std::string diagnostic = diagnosticText(result);
        std::string message = result.success()
                                  ? "Signature valide.\n\n"
                                  : "Signature invalide ou non vérifiable.\n\n";
        textResultBuffer_->text((message + diagnostic).c_str());
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
    encryptionFingerprintOutput_->value(settings_.keys.encryptionFingerprint.empty()
                                            ? "Aucune clé de chiffrement sélectionnée"
                                            : settings_.keys.encryptionFingerprint.c_str());
    signingFingerprintOutput_->value(settings_.keys.signingFingerprint.empty()
                                         ? "Aucune clé de signature sélectionnée"
                                         : settings_.keys.signingFingerprint.c_str());
    saveSettings();
}

void MainWindow::appendLog(const std::string& message) {
    if (!logBuffer_) {
        return;
    }
    auto line = currentTimestamp() + "  " + message + "\n";
    logBuffer_->append(line.c_str());
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
