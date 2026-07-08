#include "IniFile.hpp"
#include "SettingsService.hpp"

#include <cassert>
#include <iostream>

void runKeyParserTests();
void runGpgArgumentTests();
void runSealKeyPathTests();

namespace {
void testIniParsing() {
    IniFile ini;
    assert(ini.parse(R"(
; comment
[window]
x=42
width=bad

[options]
armor=yes
)"));
    assert(ini.getInt("window", "x", 0) == 42);
    assert(ini.getInt("window", "width", 980) == 980);
    assert(ini.getBool("options", "armor", false));
    assert(ini.getString("missing", "value", "fallback") == "fallback");
}

void testIniSerialization() {
    IniFile ini;
    ini.set("gpg", "executable", "/usr/bin/gpg");
    ini.setBool("options", "armor", true);
    auto text = ini.serialize();
    IniFile parsed;
    assert(parsed.parse(text));
    assert(parsed.getString("gpg", "executable", "") == "/usr/bin/gpg");
    assert(parsed.getBool("options", "armor", false));
}

void testSettingsDefaults() {
    AppSettings settings;
    assert(!settings.options.encryptAndSign);
    assert(settings.options.encryptedFileExtension == "gpg");
    assert(settings.options.signatureFileExtension == "sig");
    assert(settings.options.privateKeyColumnWidths == "110,155,85,285,70,85");
    assert(settings.options.recipientKeyColumnWidths == "110,155,85,285,70,85");
    assert(settings.options.encryptRecipientColumnWidths == "110,155,85,285,70,85");
    assert(settings.options.signerColumnWidths == "180,180,160,100,150");
    settings.gpg.executablePath = "/usr/bin/gpg";
    settings.keys.signingFingerprint = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    assert(settings.gpg.executablePath == "/usr/bin/gpg");
    assert(settings.keys.signingFingerprint == "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
}
}

int main() {
    testIniParsing();
    testIniSerialization();
    testSettingsDefaults();
    runKeyParserTests();
    runGpgArgumentTests();
    runSealKeyPathTests();
    std::cout << "sealkey_tests passed\n";
    return 0;
}
