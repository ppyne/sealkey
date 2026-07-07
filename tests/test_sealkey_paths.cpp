#include "SealKeyPaths.hpp"

#include <cassert>

void runSealKeyPathTests() {
    assert(SealKeyPaths::normalizeExtension("gpg", "gpg") == "gpg");
    assert(SealKeyPaths::normalizeExtension(".gpg", "gpg") == "gpg");
    assert(SealKeyPaths::normalizeExtension("  gpg  ", "gpg") == "gpg");
    assert(SealKeyPaths::normalizeExtension(" .sig ", "sig") == "sig");
    assert(SealKeyPaths::normalizeExtension("   ", "gpg") == "gpg");

    assert(SealKeyPaths::appendExtension("/tmp/document.pdf", "gpg") == "/tmp/document.pdf.gpg");
    assert(SealKeyPaths::appendExtension("/tmp/document.pdf", ".sealed") == "/tmp/document.pdf.sealed");
    assert(SealKeyPaths::removeExtensionForDecrypt("/tmp/document.pdf.gpg", "gpg") == "/tmp/document.pdf");
    assert(SealKeyPaths::removeExtensionForDecrypt("/tmp/document.pdf.sealed", "sealed") == "/tmp/document.pdf");
    assert(SealKeyPaths::removeExtensionForDecrypt("/tmp/document.pdf", "gpg") == "/tmp/document.pdf.decrypted");
    assert(SealKeyPaths::signaturePathFor("/tmp/document.pdf", "sig") == "/tmp/document.pdf.sig");
}
