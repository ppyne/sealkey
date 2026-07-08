#include "CryptoService.hpp"
#include "KeyStore.hpp"
#include "SignatureService.hpp"

#include <cassert>
#include <vector>

void runGpgArgumentTests() {
    const std::string fingerprint = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

    auto encryptArgs = CryptoService::encryptTextArguments(fingerprint, true);
    assert((encryptArgs == std::vector<std::string>{"--armor", "--encrypt", "--recipient", fingerprint}));

    auto decryptArgs = CryptoService::decryptTextArguments();
    assert((decryptArgs == std::vector<std::string>{"--decrypt"}));

    auto signArgs = SignatureService::signTextArguments(fingerprint);
    assert((signArgs == std::vector<std::string>{"--clearsign", "--local-user", fingerprint}));

    auto verifyArgs = SignatureService::verifyTextArguments();
    assert((verifyArgs == std::vector<std::string>{"--verify"}));

    auto encryptFileArgs = CryptoService::encryptFileArguments("plain.txt", "plain.txt.asc", fingerprint, true);
    assert((encryptFileArgs == std::vector<std::string>{
                                  "--yes", "--batch", "--trust-model", "always", "--armor", "--encrypt",
                                  "--recipient", fingerprint, "--output", "plain.txt.asc", "plain.txt"}));

    auto decryptFileArgs = CryptoService::decryptFileArguments("plain.txt.asc", "plain.txt");
    assert((decryptFileArgs == std::vector<std::string>{
                                  "--yes", "--decrypt", "--output", "plain.txt", "plain.txt.asc"}));

    auto inspectFileArgs = CryptoService::inspectEncryptedFileArguments("plain.txt.asc");
    assert(inspectFileArgs.size() == 7);
    assert(inspectFileArgs[0] == "--status-fd");
    assert(inspectFileArgs[1] == "1");
    assert(inspectFileArgs[2] == "--yes");
    assert(inspectFileArgs[3] == "--decrypt");
    assert(inspectFileArgs[4] == "--output");
    assert(inspectFileArgs[6] == "plain.txt.asc");

    auto encryptSignArgs = CryptoService::encryptAndSignFileArguments("plain.txt", "plain.txt.gpg", fingerprint, fingerprint, true);
    assert((encryptSignArgs == std::vector<std::string>{
                                  "--yes", "--batch", "--trust-model", "always", "--armor", "--encrypt", "--sign",
                                  "--recipient", fingerprint, "--local-user", fingerprint,
                                  "--output", "plain.txt.gpg", "plain.txt"}));

    auto signFileArgs = SignatureService::signFileDetachedArguments("plain.txt", "plain.txt.asc.sig", fingerprint);
    assert((signFileArgs == std::vector<std::string>{
                              "--yes", "--no-tty", "--armor", "--detach-sign", "--local-user", fingerprint,
                              "--output", "plain.txt.asc.sig", "plain.txt"}));

    auto verifyDetachedArgs = SignatureService::verifyDetachedFileArguments("plain.txt.asc.sig", "plain.txt");
    assert((verifyDetachedArgs == std::vector<std::string>{"--verify", "plain.txt.asc.sig", "plain.txt"}));

    auto inspectDetachedArgs = SignatureService::inspectDetachedFileArguments("plain.txt.asc.sig", "plain.txt");
    assert((inspectDetachedArgs == std::vector<std::string>{"--status-fd", "1", "--verify", "plain.txt.asc.sig", "plain.txt"}));

    auto verifySignedArgs = SignatureService::verifySignedFileArguments("signed.asc");
    assert((verifySignedArgs == std::vector<std::string>{"--verify", "signed.asc"}));

    auto generateKeyArgs = KeyStore::generatePrivateKeyArguments("Alice Example <alice@example.test>", "1y", "default");
    assert((generateKeyArgs == std::vector<std::string>{
                                  "--batch", "--yes", "--no-tty", "--quick-generate-key",
                                  "Alice Example <alice@example.test>", "default", "default", "1y"}));

    auto rsa4096Args = KeyStore::generatePrivateKeyArguments("Alice Example <alice@example.test>", "1y", "rsa4096");
    assert((rsa4096Args == std::vector<std::string>{
                              "--batch", "--yes", "--no-tty", "--quick-generate-key",
                              "Alice Example <alice@example.test>", "rsa4096", "default", "1y"}));

    auto ed25519Args = KeyStore::generatePrivateKeyArguments("Alice Example <alice@example.test>", "1y", "ed25519-cv25519");
    assert((ed25519Args == std::vector<std::string>{
                              "--batch", "--yes", "--no-tty", "--quick-generate-key",
                              "Alice Example <alice@example.test>", "ed25519", "cert,sign", "1y"}));

    auto cv25519Args = KeyStore::addEncryptionSubkeyArguments(fingerprint, "1y", "ed25519-cv25519");
    assert((cv25519Args == std::vector<std::string>{
                              "--batch", "--yes", "--no-tty", "--quick-add-key",
                              fingerprint, "cv25519", "encrypt", "1y"}));

    auto profiles = KeyStore::availableKeyGenerationProfiles("Supported algorithms:\nPubkey: RSA, ECDH, ECDSA, EDDSA\n");
    assert(profiles.size() == 4);
    assert(profiles.back().id == "ed25519-cv25519");
    assert(profiles.back().requiresEncryptionSubkey);

    auto rsaOnlyProfiles = KeyStore::availableKeyGenerationProfiles("Supported algorithms:\nPubkey: RSA, DSA\n");
    assert(rsaOnlyProfiles.size() == 3);

    GpgProcessResult verification;
    verification.exitCode = 0;
    verification.standardError =
        "gpg: Signature made Thu Jul  2 10:00:00 2026 CEST\n"
        "gpg:                using EDDSA key 1FD99CF3B9B8641A17FCC08AE88E0BF0E63DDF8D\n"
        "gpg: Good signature from \"SealKey Test <sealkey@example.test>\" [ultimate]\n"
        "gpg: Primary key fingerprint: 1FD9 9CF3 B9B8 641A 17FC  C08A E88E 0BF0 E63D DF8D\n";
    auto summary = SignatureService::summarizeVerification(verification);
    assert(summary.valid);
    assert(summary.fingerprint == "1FD99CF3B9B8641A17FCC08AE88E0BF0E63DDF8D");

    GpgProcessResult unknown;
    unknown.exitCode = 2;
    unknown.standardError = "gpg: Can't check signature: No public key\n";
    auto unknownSummary = SignatureService::summarizeVerification(unknown);
    assert(!unknownSummary.valid);
    assert(unknownSummary.unknownKey);
}
