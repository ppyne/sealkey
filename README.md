# SealKey

SealKey is a lightweight FLTK desktop helper for the local `gpg` executable.

Version 0.1 provides:

- a C++20/CMake project structure;
- an FLTK application named `sealkey`;
- Configuration, Clés, Texte, Fichiers and Journal tabs;
- non-sensitive INI preferences in the standard user configuration directory;
- window geometry persistence;
- `gpg` executable selection, auto-detection and `gpg --version` test;
- public and secret key listing via `--with-colons`;
- separate encryption and signing key selections by full fingerprint;
- ASCII-armored public-key export for the selected encryption key;
- last-used open/save/export directories.

The application calls `gpg` as an external process and passes arguments as a vector. It does not use Qt, wxWidgets, GTK, Electron, libgpgme or OpenSSL, and it does not store passphrases or secrets.

## Build

Install CMake, a C++20 compiler and FLTK 1.4 or compatible.

```bash
cmake -S . -B build -DUSE_SYSTEM_FLTK=ON -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/sealkey
```

For vendored/static FLTK, place FLTK in `third_party/fltk` or allow CMake to fetch it:

```bash
cmake -S . -B build -DUSE_SYSTEM_FLTK=OFF -DBUILD_STATIC=ON
cmake --build build
```

Network access may be required when `USE_SYSTEM_FLTK=OFF` and `third_party/fltk` is absent.

## Preferences

SealKey stores only non-sensitive preferences:

- Linux: `$XDG_CONFIG_HOME/sealkey/preferences.ini` or `~/.config/sealkey/preferences.ini`;
- macOS: `~/Library/Application Support/sealkey/preferences.ini`;
- Windows: `%APPDATA%\sealkey\preferences.ini`.

Stored values include window geometry, selected `gpg` executable, selected full fingerprints and last-used directories.

## v0.1 Limits

The Texte and Fichiers tabs are present but intentionally inactive. Text encryption/decryption, file encryption/decryption, signing and verification are planned for later versions after v0.1 is stable and documented.
