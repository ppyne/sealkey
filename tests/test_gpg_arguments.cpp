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
}
