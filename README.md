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

## Build and Run

Install CMake, a C++20 compiler and FLTK 1.4 or compatible.

For vendored/static FLTK, place FLTK in `third_party/fltk` or allow CMake to fetch it:

```bash
cmake -S . -B build -DUSE_SYSTEM_FLTK=OFF -DBUILD_STATIC=ON -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Network access may be required when `USE_SYSTEM_FLTK=OFF` and `third_party/fltk` is absent.

To use an already installed FLTK package instead:

```bash
cmake -S . -B build -DUSE_SYSTEM_FLTK=ON -DENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Linux

The built executable is available at:

```bash
./build/sealkey
```

For normal desktop use, install it into a prefix so the `.desktop` file and icons are installed too:

```bash
cmake --install build --prefix "$HOME/.local"
```

Then run it from the application menu or directly:

```bash
"$HOME/.local/bin/sealkey"
```

The install step matters on Linux because taskbars and application menus use the installed desktop entry and icon theme files:

- `$HOME/.local/share/applications/sealkey.desktop`;
- `$HOME/.local/share/icons/hicolor/.../apps/sealkey.png`;
- `$HOME/.local/share/icons/hicolor/scalable/apps/sealkey.svg`.

If the taskbar still shows an old or wrong icon after reinstalling, refresh the desktop/icon caches if those tools are available:

```bash
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
```

On lightweight desktops, including Raspberry Pi OS/LXPanel, the panel may keep an old icon association until the session or panel is restarted. Launching SealKey from an editor or IDE can also make some panels associate the window with the parent application; launching the installed `sealkey.desktop` entry is the preferred test.

### macOS

The CMake build creates a macOS application bundle. Launch the bundle, not the raw executable:

```bash
open build/sealkey.app
```

From Finder, open:

```text
build/sealkey.app
```

SealKey does not bundle `gpg`. Install GnuPG separately, then select the `gpg` executable in SealKey if auto-detection does not find it. Common paths include:

```text
/opt/homebrew/bin/gpg
/usr/local/bin/gpg
/opt/local/bin/gpg
```

### Windows

With a Windows generator, build the project normally:

```powershell
cmake -S . -B build -DUSE_SYSTEM_FLTK=OFF -DBUILD_STATIC=ON -DENABLE_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The GUI executable is generated under the build directory. With multi-configuration generators such as Visual Studio, the release executable is typically:

```text
build\Release\sealkey.exe
```

With single-configuration generators such as Ninja or MinGW Makefiles, it is typically:

```text
build\sealkey.exe
```

SealKey does not bundle `gpg`. Install GnuPG separately, then select `gpg.exe` in SealKey if auto-detection does not find it.

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
