#include "CryptoService.hpp"
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
                                  "--yes", "--batch", "--armor", "--encrypt", "--recipient", fingerprint,
                                  "--output", "plain.txt.asc", "plain.txt"}));

    auto decryptFileArgs = CryptoService::decryptFileArguments("plain.txt.asc", "plain.txt");
    assert((decryptFileArgs == std::vector<std::string>{
                                  "--yes", "--decrypt", "--output", "plain.txt", "plain.txt.asc"}));

    auto signFileArgs = SignatureService::signFileDetachedArguments("plain.txt", "plain.txt.asc.sig", fingerprint);
    assert((signFileArgs == std::vector<std::string>{
                              "--armor", "--detach-sign", "--local-user", fingerprint,
                              "--output", "plain.txt.asc.sig", "plain.txt"}));

    auto verifyDetachedArgs = SignatureService::verifyDetachedFileArguments("plain.txt.asc.sig", "plain.txt");
    assert((verifyDetachedArgs == std::vector<std::string>{"--verify", "plain.txt.asc.sig", "plain.txt"}));

    auto verifySignedArgs = SignatureService::verifySignedFileArguments("signed.asc");
    assert((verifySignedArgs == std::vector<std::string>{"--verify", "signed.asc"}));
}
