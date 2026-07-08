![SealKey](icons/linux_icons/sealkey_128x128.png "SealKey").

# SealKey

SealKey is a lightweight FLTK desktop assistant for common local `gpg` workflows.

Version 0.6.1 builds on the v0.6 interface with five direct GPG operation tabs:

- a C++20/CMake project structure;
- an FLTK application named `sealkey`;
- five main tabs: Crypter, Décrypter, Signer, Vérifier and Configuration;
- non-sensitive INI preferences in the standard user configuration directory;
- window geometry persistence;
- `gpg` executable selection, auto-detection and `gpg --version` test;
- public and secret key listing via `--with-colons`;
- private signing-key selection and recipient public-key selection by full fingerprint;
- a separate preferred private key used for signing operations;
- configurable encrypted-file and signature extensions, defaulting to `gpg` and `sig`;
- ASCII-armored public-key export for the selected private key or selected recipient;
- explicit private-key export after confirmation;
- explicit private/public key deletion after confirmation;
- public-key import for recipients;
- private-key creation through `gpg --quick-generate-key`, with trimmed identity fields, selectable key profiles and optional ownertrust import;
- last-used open/save/export directories;
- file encryption through `gpg --yes --batch --armor --encrypt --recipient <FINGERPRINT> --output <DESTINATION> <SOURCE>`;
- file encryption and signing through `gpg --yes --batch --armor --encrypt --sign --recipient <FINGERPRINT> --local-user <FINGERPRINT> --output <DESTINATION> <SOURCE>`;
- encryption uses the explicitly selected recipient fingerprint and `--trust-model always` to avoid interactive trust prompts in batch mode;
- file decryption through `gpg --yes --decrypt --output <DESTINATION> <SOURCE>`;
- detached ASCII file signing through `gpg --armor --detach-sign --local-user <FINGERPRINT> --output <SIGNATURE> <SOURCE>`;
- detached signature verification through `gpg --verify <SIGNATURE> <SOURCE>`;
- clearer signature diagnostics for valid, invalid, unknown-key, expired, revoked and untrusted-key cases;
- full last-tab restoration;
- simple business objects for local identities, contacts, user operations and operation results;
- result displays with clear user messages and technical GPG details;
- platform icons for Windows, macOS and Linux, including a runtime Linux window icon and a desktop entry with a matching window class for taskbar icon association.

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

Stored values include window geometry, last tab, selected `gpg` executable, selected full fingerprints, last-used directories, ASCII armor preference, encrypt-and-sign preference, preferred signature type and the configured encrypted-file/signature extensions.

## Current Limits

File operations pass explicit source, destination and signature paths to `gpg` as argument vectors. SealKey does not store source text, encrypted text, signatures, passphrases, file contents or operation outputs in preferences.

The v0.6 interface is intentionally compact and native. Recipient selection is a direct list of public encryption keys rather than a full address book.

Remaining work before 1.0 is mostly distribution quality and polish: broader packaging, broader integration tests, platform-specific build documentation, drag-and-drop and a richer contact/address-book view.
