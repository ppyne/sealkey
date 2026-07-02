#pragma once

#include "GpgKey.hpp"
#include "SettingsService.hpp"

#include <FL/Fl_Window.H>

#include <string>
#include <vector>

class Fl_Box;
class Fl_Button;
class Fl_Choice;
class Fl_Group;
class Fl_Hold_Browser;
class Fl_Input;
class Fl_Output;
class Fl_Tabs;
class Fl_Text_Buffer;
class Fl_Text_Display;
class Fl_Text_Editor;

class MainWindow : public Fl_Window {
public:
    MainWindow();
    ~MainWindow() override;

private:
    void buildInterface();
    void loadInitialState();
    void saveSettings();
    void updateGpgPath(const std::string& path, bool saveNow);
    void chooseGpgExecutable();
    void autoDetectGpg();
    void testGpg();
    void reloadKeys();
    void exportEncryptionPublicKey();
    void executeTextOperation();
    void copyTextResult();
    void clearTextBuffers();
    void saveTextResult();
    void chooseFileSource();
    void chooseFileDestination();
    void chooseSignatureFile(bool saveMode);
    void executeFileOperation();
    void populateKeyBrowsers();
    void restoreKeySelections();
    void appendLog(const std::string& message);
    void updateStatus(const std::string& message);
    std::string selectedEncryptionFingerprint() const;
    std::string selectedSigningFingerprint() const;
    void updateLastTab();

    static void onClose(Fl_Widget* widget, void* data);

    SettingsService settingsService_;
    AppSettings settings_;
    std::vector<GpgKey> encryptionKeys_;
    std::vector<GpgKey> signingKeys_;

    Fl_Tabs* tabs_ = nullptr;
    Fl_Group* configurationTab_ = nullptr;
    Fl_Group* keysTab_ = nullptr;
    Fl_Group* textTab_ = nullptr;
    Fl_Group* filesTab_ = nullptr;
    Fl_Group* logTab_ = nullptr;
    Fl_Input* gpgPathInput_ = nullptr;
    Fl_Output* gpgStatusOutput_ = nullptr;
    Fl_Hold_Browser* encryptionBrowser_ = nullptr;
    Fl_Hold_Browser* signingBrowser_ = nullptr;
    Fl_Output* encryptionFingerprintOutput_ = nullptr;
    Fl_Output* signingFingerprintOutput_ = nullptr;
    Fl_Choice* textOperationChoice_ = nullptr;
    Fl_Text_Buffer* textSourceBuffer_ = nullptr;
    Fl_Text_Buffer* textResultBuffer_ = nullptr;
    Fl_Text_Editor* textSourceEditor_ = nullptr;
    Fl_Text_Display* textResultDisplay_ = nullptr;
    Fl_Choice* fileOperationChoice_ = nullptr;
    Fl_Input* fileSourceInput_ = nullptr;
    Fl_Input* fileDestinationInput_ = nullptr;
    Fl_Input* fileSignatureInput_ = nullptr;
    Fl_Output* fileStatusOutput_ = nullptr;
    Fl_Text_Buffer* logBuffer_ = nullptr;
    Fl_Text_Display* logDisplay_ = nullptr;
};
