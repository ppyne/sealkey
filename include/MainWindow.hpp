#pragma once

#include "GpgKey.hpp"
#include "SettingsService.hpp"

#include <FL/Fl_Window.H>

#include <array>
#include <string>
#include <vector>

class Fl_Button;
class Fl_Check_Button;
class Fl_Choice;
class Fl_Group;
class Fl_Hold_Browser;
class Fl_Input;
class Fl_Output;
class Fl_Tabs;
class Fl_Text_Buffer;
class Fl_Text_Display;
struct GpgProcessResult;

class MainWindow : public Fl_Window {
public:
    MainWindow();
    ~MainWindow() override;

private:
    void buildInterface();
    void loadInitialState();
    void saveSettings();
    void reloadKeys();
    void populateKeyBrowsers();
    void restoreKeySelections();
    void updateActions();
    void updateLastTab();

    void chooseGpgExecutable();
    void autoDetectGpg();
    void testGpg();
    void chooseEncryptFile();
    void executeEncrypt();
    void chooseDecryptFile();
    void refreshDecryptSigners();
    void executeDecrypt();
    void chooseSignFile();
    void executeSign();
    void chooseVerifyFile();
    void chooseVerifySignature();
    void refreshVerifySigners();
    void executeVerify();
    void importRecipientKey();
    void exportSelectedRecipientKey();
    void exportMyPublicKey();
    void importPrivateKey();
    void exportPrivateKey();
    void deleteSelectedPrivateKey();
    void deleteSelectedRecipientKey();
    void showSelectedPrivateKeyInfo();
    void setPreferredPrivateKey();
    void createPrivateKey();
    void normalizeAndSaveEncryptedExtension();
    void normalizeAndSaveSignatureExtension();
    void loadPrivateKeyColumnWidths();
    void savePrivateKeyColumnWidths();
    void loadRecipientKeyColumnWidths();
    void saveRecipientKeyColumnWidths();
    void loadEncryptRecipientColumnWidths();
    void saveEncryptRecipientColumnWidths();
    void loadSignerColumnWidths();
    void saveSignerColumnWidths();

    const GpgKey* selectedEncryptRecipientKey() const;
    const GpgKey* selectedConfigRecipientKey() const;
    const GpgKey* selectedListedSigningKey() const;
    const GpgKey* selectedSigningKey() const;
    bool gpgConfigured() const;
    Fl_Text_Buffer* styleBufferFor(Fl_Text_Buffer* buffer) const;
    void setResult(Fl_Text_Buffer* buffer, const std::string& text);
    void setResult(Fl_Text_Buffer* buffer, const std::string& text, const GpgProcessResult& result);
    void setSigners(Fl_Hold_Browser* browser, const std::string& text);
    void updateStatus(const std::string& message);
    void flashStatus();
    void updatePreferredKeyOutput();
    bool confirmOverwrite(const std::string& path);
    std::string selectedSigningLabel() const;

    static void onClose(Fl_Widget* widget, void* data);
    static void onStatusFlash(void* data);

    SettingsService settingsService_;
    AppSettings settings_;
    std::vector<GpgKey> recipientKeys_;
    std::vector<GpgKey> signingKeys_;
    bool gpgWorks_ = false;

    Fl_Tabs* tabs_ = nullptr;
    Fl_Group* encryptTab_ = nullptr;
    Fl_Group* decryptTab_ = nullptr;
    Fl_Group* signTab_ = nullptr;
    Fl_Group* verifyTab_ = nullptr;
    Fl_Group* configurationTab_ = nullptr;

    Fl_Input* encryptFileInput_ = nullptr;
    Fl_Widget* encryptRecipientHeader_ = nullptr;
    Fl_Hold_Browser* encryptRecipientBrowser_ = nullptr;
    std::array<int, 7> encryptRecipientColumnWidths_ = {110, 155, 85, 285, 70, 85, 0};
    Fl_Check_Button* encryptSignCheck_ = nullptr;
    Fl_Button* encryptButton_ = nullptr;
    Fl_Text_Buffer* encryptResultBuffer_ = nullptr;
    Fl_Text_Buffer* encryptResultStyleBuffer_ = nullptr;

    Fl_Input* decryptFileInput_ = nullptr;
    Fl_Widget* decryptSignersHeader_ = nullptr;
    Fl_Hold_Browser* decryptSignersBrowser_ = nullptr;
    std::array<int, 6> signerColumnWidths_ = {180, 180, 160, 100, 150, 0};
    Fl_Button* decryptButton_ = nullptr;
    Fl_Text_Buffer* decryptResultBuffer_ = nullptr;
    Fl_Text_Buffer* decryptResultStyleBuffer_ = nullptr;

    Fl_Input* signFileInput_ = nullptr;
    Fl_Button* signButton_ = nullptr;
    Fl_Text_Buffer* signResultBuffer_ = nullptr;
    Fl_Text_Buffer* signResultStyleBuffer_ = nullptr;

    Fl_Input* verifyFileInput_ = nullptr;
    Fl_Input* verifySignatureInput_ = nullptr;
    Fl_Widget* verifySignersHeader_ = nullptr;
    Fl_Hold_Browser* verifySignersBrowser_ = nullptr;
    Fl_Button* verifyButton_ = nullptr;
    Fl_Text_Buffer* verifyResultBuffer_ = nullptr;
    Fl_Text_Buffer* verifyResultStyleBuffer_ = nullptr;

    Fl_Input* gpgPathInput_ = nullptr;
    Fl_Output* gpgStatusOutput_ = nullptr;
    int statusFlashRemaining_ = 0;
    Fl_Hold_Browser* myKeyBrowser_ = nullptr;
    Fl_Widget* privateKeyHeader_ = nullptr;
    std::array<int, 7> privateKeyColumnWidths_ = {110, 155, 85, 285, 70, 85, 0};
    Fl_Output* preferredKeyOutput_ = nullptr;
    Fl_Button* setPreferredKeyButton_ = nullptr;
    Fl_Button* exportMyKeyButton_ = nullptr;
    Fl_Button* importPrivateKeyButton_ = nullptr;
    Fl_Button* exportPrivateKeyButton_ = nullptr;
    Fl_Button* infoPrivateKeyButton_ = nullptr;
    Fl_Button* deletePrivateKeyButton_ = nullptr;
    Fl_Hold_Browser* recipientsBrowser_ = nullptr;
    Fl_Widget* recipientKeyHeader_ = nullptr;
    std::array<int, 7> recipientKeyColumnWidths_ = {110, 155, 85, 285, 70, 85, 0};
    Fl_Button* exportRecipientButton_ = nullptr;
    Fl_Button* deleteRecipientButton_ = nullptr;
    Fl_Input* encryptedExtensionInput_ = nullptr;
    Fl_Input* signatureExtensionInput_ = nullptr;
};
