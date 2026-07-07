# SealKey

SealKey is a lightweight FLTK desktop assistant for common local `gpg` workflows.

Version 0.5 shifts the product toward human use cases:

- a C++20/CMake project structure;
- an FLTK application named `sealkey`;
- an Accueil screen organized around intentions rather than raw GPG verbs;
- guided Envoyer, Recevoir and Préparer flows;
- preserved advanced Configuration, Clés, Texte, Fichiers and Journal tabs;
- non-sensitive INI preferences in the standard user configuration directory;
- window geometry persistence;
- `gpg` executable selection, auto-detection and `gpg --version` test;
- public and secret key listing via `--with-colons`;
- separate encryption and signing key selections by full fingerprint;
- ASCII-armored public-key export for the selected encryption key;
- last-used open/save/export directories;
- text encryption through `gpg --armor --encrypt --recipient <FINGERPRINT>`;
- text decryption through `gpg --decrypt`;
- clear text signing through `gpg --clearsign --local-user <FINGERPRINT>`;
- signed text verification through `gpg --verify`;
- text result copy, clearing and save-to-file;
- file encryption through `gpg --yes --batch --armor --encrypt --recipient <FINGERPRINT> --output <DESTINATION> <SOURCE>`;
- file decryption through `gpg --yes --decrypt --output <DESTINATION> <SOURCE>`;
- detached ASCII file signing through `gpg --armor --detach-sign --local-user <FINGERPRINT> --output <SIGNATURE> <SOURCE>`;
- detached signature verification through `gpg --verify <SIGNATURE> <SOURCE>`;
- signed-file verification through `gpg --verify <SIGNED_FILE>`;
- clearer signature diagnostics for valid, invalid, unknown-key, expired, revoked and untrusted-key cases;
- visible warnings before using expired, revoked or not fully trusted keys;
- explicit clipboard clearing;
- journal clearing;
- full last-tab restoration and safer restored window placement;
- simple business objects for local identities, contacts, user operations and operation results;
- result displays with a clear user message and separate technical GPG details;
- public-key import from the main workflow;
- package generation with protected files, optional sender public key and README.txt.

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

Stored values include window geometry, last tab, selected `gpg` executable, selected full fingerprints, last-used directories, ASCII armor preference and preferred signature type.

## Current Limits

Text operations use stdin/stdout. File operations pass explicit source, destination and signature paths to `gpg` as argument vectors. SealKey does not store source text, encrypted text, signatures, passphrases, file contents or operation outputs in preferences. The Journal tab records only short operation status messages.

The v0.5 assistants are intentionally simple first iterations. The package flow supports multiple files by adding them one by one, and recipient selection is a direct list of public encryption keys rather than a full address book.

Remaining work before 1.0 is mostly distribution quality and polish: platform packaging, icon, broader integration tests, platform-specific build documentation, drag-and-drop and a richer contact/address-book view.
