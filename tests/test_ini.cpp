#include "IniFile.hpp"
#include "SettingsService.hpp"

#include <cassert>
#include <iostream>

void runKeyParserTests();

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
}

int main() {
    testIniParsing();
    testIniSerialization();
    runKeyParserTests();
    std::cout << "sealkey_tests passed\n";
    return 0;
}
