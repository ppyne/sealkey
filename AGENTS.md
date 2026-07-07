# AGENTS.md — SealKey

## Project

SealKey — Lightweight GPG desktop helper.

Read `specification.md` before making any change. It is the source of truth for the project.

## Technical constraints

* Language: C++20.
* Build system: CMake.
* GUI toolkit: FLTK.
* Prefer static FLTK linking.
* Do not use Qt.
* Do not use wxWidgets.
* Do not use GTK.
* Do not use Electron.
* Do not use libgpgme.
* Do not use OpenSSL.
* Do not add heavy dependencies unless explicitly justified.

## GPG integration

SealKey must call the external `gpg` executable.

Never build shell commands as a single string.

Always pass arguments safely as an argument vector.

Use full fingerprints, never short key IDs, for GPG operations.

Do not implement cryptography directly.

## Security rules

Never store:

* passphrases;
* private keys;
* plaintext content;
* encrypted text content;
* signatures;
* full operation outputs containing user content.

Only store non-sensitive preferences:

* window geometry;
* selected `gpg` executable path;
* selected encryption key fingerprint;
* selected signing key fingerprint;
* last used directories;
* UI options.

## Development order

Implement version 0.1 first.

Do not start text encryption, file encryption, signing, or verification features until v0.1 is complete, compilable, and documented.

## Code quality

Keep the code modular.

Prefer small classes with clear responsibilities.

Avoid platform-specific code outside dedicated abstraction modules.

Document build instructions in `README.md`.

When changing behavior, update documentation if needed.
