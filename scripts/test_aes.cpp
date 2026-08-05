// Conformance test for src/core/utils/aescrypto.cpp.
// Vectors come from an independent Python implementation (vectors.py).
// Both backends must produce identical results - run this on Windows AND macOS.
#include "aescrypto.h"
#include <QByteArray>
#include <cstdio>

static const char *kCtrKey    = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
static const char *kCtrIn     = "fffffffffffffffffffffffffffffffd";
static const char *kCtrPlain  = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445";
static const char *kCtrCipher = "481439fb2e7063127a700193596fa05373f4a611a10b5ecae2f4e3c585d1395bc9b8c63e688256fd7bae3b5071a279c1c2a132851e7ca9e791caa051e11349bfb01c34ed0efc";
static const char *kGcmKey    = "00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff";
static const char *kGcmIv     = "000102030405060708090a0b";
static const char *kGcmCt     = "5777fb572d20eddba85820af11a50b5d1a0f873fbe5756d87a6f61af4b8c4c5890a9d404818af151ff3592ee9e957cb93067fb539c3e4f97c310e9c72cf0";
static const char *kGcmPlain  = "AoNami macOS port: AES-GCM conformance vector.";

static int failures = 0;

static void check(const char *what, bool ok) {
    std::printf("  %-46s %s\n", what, ok ? "PASS" : "FAIL");
    if (!ok) ++failures;
}

static QByteArray hex(const char *h) { return QByteArray::fromHex(h); }

int main() {
    std::printf("AesCrypto conformance\n\n");

    // CTR encrypt matches the reference keystream.
    const QByteArray ct = AesCrypto::ctr(hex(kCtrKey), hex(kCtrIn), hex(kCtrPlain));
    check("CTR encrypt matches reference", ct.toHex() == QByteArray(kCtrCipher));

    // CTR is its own inverse - this is what the provider actually does.
    const QByteArray back = AesCrypto::ctr(hex(kCtrKey), hex(kCtrIn), hex(kCtrCipher));
    check("CTR decrypt round-trips", back.toHex() == QByteArray(kCtrPlain));

    // The counter must carry across bytes; kCtrIn is ...fffd so block 3 wraps to zero.
    check("CTR non-block-aligned length preserved", ct.size() == hex(kCtrPlain).size());

    // Wrong key sizes are rejected rather than producing garbage.
    check("CTR rejects short key", AesCrypto::ctr(hex("00112233"), hex(kCtrIn), hex(kCtrPlain)).isEmpty());
    check("CTR rejects short counter", AesCrypto::ctr(hex(kCtrKey), hex("0011"), hex(kCtrPlain)).isEmpty());

    // GCM decrypt + tag verification.
    const QByteArray pt = AesCrypto::gcmDecrypt(hex(kGcmKey), hex(kGcmIv), hex(kGcmCt));
    check("GCM decrypt matches reference", pt == QByteArray(kGcmPlain));

    // A flipped tag byte must fail, not return plaintext.
    QByteArray tampered = hex(kGcmCt);
    tampered[tampered.size() - 1] = tampered[tampered.size() - 1] ^ 0x01;
    check("GCM rejects tampered tag", AesCrypto::gcmDecrypt(hex(kGcmKey), hex(kGcmIv), tampered).isEmpty());

    // A flipped ciphertext byte must also fail authentication.
    QByteArray corrupt = hex(kGcmCt);
    corrupt[0] = corrupt[0] ^ 0x01;
    check("GCM rejects corrupted ciphertext", AesCrypto::gcmDecrypt(hex(kGcmKey), hex(kGcmIv), corrupt).isEmpty());

    check("GCM rejects undersized input", AesCrypto::gcmDecrypt(hex(kGcmKey), hex(kGcmIv), hex("0011")).isEmpty());

    std::printf("\n%s\n", failures ? "SOME CHECKS FAILED" : "all checks passed");
    return failures ? 1 : 0;
}
