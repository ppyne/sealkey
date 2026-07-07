#include "KeyStore.hpp"
#include "SealKeyModels.hpp"

#include <cassert>

void runKeyParserTests() {
    const std::string publicListing =
        "pub:u:255:22:1234567890ABCDEF:1700000000:1800000000::u:::scESC::::::23::0:\n"
        "fpr:::::::::AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA:\n"
        "uid:u::::1700000001::HASH::Alice Example <alice@example.test>::::::::::0:\n"
        "sub:u:255:18:FEDCBA0987654321:1700000002:1800000002:::::e::::::23:\n"
        "fpr:::::::::BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB:\n";

    auto publicKeys = KeyStore::parseColonListing(publicListing, false);
    assert(publicKeys.size() == 1);
    assert(publicKeys[0].fingerprint == "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    assert(publicKeys[0].keyId == "1234567890ABCDEF");
    assert(publicKeys[0].uid == "Alice Example <alice@example.test>");
    assert(publicKeys[0].email == "alice@example.test");
    assert(publicKeys[0].canEncrypt);
    assert(publicKeys[0].canSign);
    assert(publicKeys[0].canCertify);
    assert(!publicKeys[0].hasSecretKey);

    const std::string secretListing =
        "sec:u:255:22:ABCDEF1234567890:1700000000::::::sc::::::23::0:\n"
        "fpr:::::::::CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC:\n"
        "uid:u::::1700000001::HASH::Bob Example <bob@example.test>::::::::::0:\n";

    auto secretKeys = KeyStore::parseColonListing(secretListing, true);
    assert(secretKeys.size() == 1);
    assert(secretKeys[0].hasSecretKey);
    assert(secretKeys[0].canSign);
    assert(!secretKeys[0].canEncrypt);

    auto contact = contactFromKey(publicKeys[0]);
    assert(contact.name == "Alice Example");
    assert(contact.email == "alice@example.test");
    assert(contact.canEncrypt);
    assert(shortFingerprint(contact.fingerprint) == "AAAAAAAA");

    auto identity = identityFromKey(secretKeys[0]);
    assert(identity.name == "Bob Example");
    assert(identity.canSign);
}
