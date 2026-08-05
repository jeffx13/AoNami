#include "aescrypto.h"
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <bcrypt.h>
#else
#include <CommonCrypto/CommonCryptor.h>
#endif

namespace {

// Single-block ECB encryptor. CTR is built on top of it so the counter arithmetic
// stays in one place rather than being duplicated per backend.
class EcbEncryptor {
public:
    explicit EcbEncryptor(const QByteArray &key) { open(key); }
    ~EcbEncryptor() { close(); }
    EcbEncryptor(const EcbEncryptor &) = delete;
    EcbEncryptor &operator=(const EcbEncryptor &) = delete;

    bool ok() const { return m_ok; }
    bool encryptBlock(const unsigned char *in, unsigned char *out);

private:
    void open(const QByteArray &key);
    void close();

    bool m_ok = false;
#ifdef Q_OS_WIN
    BCRYPT_ALG_HANDLE m_alg = nullptr;
    BCRYPT_KEY_HANDLE m_key = nullptr;
#else
    CCCryptorRef m_cryptor = nullptr;
#endif
};

#ifdef Q_OS_WIN

void EcbEncryptor::open(const QByteArray &key) {
    if (BCryptOpenAlgorithmProvider(&m_alg, BCRYPT_AES_ALGORITHM, nullptr, 0) != 0) return;
    if (BCryptSetProperty(m_alg, BCRYPT_CHAINING_MODE,
                          reinterpret_cast<PUCHAR>(const_cast<wchar_t *>(BCRYPT_CHAIN_MODE_ECB)),
                          sizeof(BCRYPT_CHAIN_MODE_ECB), 0) != 0) return;
    if (BCryptGenerateSymmetricKey(m_alg, &m_key, nullptr, 0,
                                   reinterpret_cast<PUCHAR>(const_cast<char *>(key.constData())),
                                   key.size(), 0) != 0) return;
    m_ok = true;
}

void EcbEncryptor::close() {
    if (m_key) BCryptDestroyKey(m_key);
    if (m_alg) BCryptCloseAlgorithmProvider(m_alg, 0);
}

bool EcbEncryptor::encryptBlock(const unsigned char *in, unsigned char *out) {
    ULONG n = 0;
    return BCryptEncrypt(m_key, const_cast<PUCHAR>(in), 16, nullptr,
                         nullptr, 0, out, 16, &n, 0) == 0;
}

#else

void EcbEncryptor::open(const QByteArray &key) {
    m_ok = CCCryptorCreate(kCCEncrypt, kCCAlgorithmAES, kCCOptionECBMode,
                           key.constData(), key.size(), nullptr, &m_cryptor) == kCCSuccess;
}

void EcbEncryptor::close() {
    if (m_cryptor) CCCryptorRelease(m_cryptor);
}

bool EcbEncryptor::encryptBlock(const unsigned char *in, unsigned char *out) {
    size_t moved = 0;
    return CCCryptorUpdate(m_cryptor, in, 16, out, 16, &moved) == kCCSuccess && moved == 16;
}

#endif

}

QByteArray AesCrypto::ctr(const QByteArray &key, QByteArray counter, const QByteArray &input) {
    if (key.size() != 32 || counter.size() != 16) return {};
    EcbEncryptor enc(key);
    if (!enc.ok()) return {};

    QByteArray out(input.size(), Qt::Uninitialized);
    unsigned char ks[16];
    for (int off = 0; off < input.size(); off += 16) {
        if (!enc.encryptBlock(reinterpret_cast<const unsigned char *>(counter.constData()), ks))
            return {};
        const int blk = qMin(16, input.size() - off);
        for (int i = 0; i < blk; ++i)
            out[off + i] = static_cast<char>(static_cast<unsigned char>(input[off + i]) ^ ks[i]);
        for (int i = 15; i >= 0; --i) {   // big-endian increment
            unsigned char v = static_cast<unsigned char>(counter[i]) + 1;
            counter[i] = static_cast<char>(v);
            if (v != 0) break;
        }
    }
    return out;
}

QByteArray AesCrypto::gcmDecrypt(const QByteArray &key, const QByteArray &iv, const QByteArray &ctWithTag) {
    constexpr int tagLen = 16;
    if (ctWithTag.size() < tagLen) return {};
    if (key.size() != 16 && key.size() != 24 && key.size() != 32) return {};

    const int ctLen = ctWithTag.size() - tagLen;
    const auto *ct  = reinterpret_cast<const unsigned char *>(ctWithTag.constData());
    const auto *tag = ct + ctLen;
    QByteArray plaintext(ctLen, 0);

#ifdef Q_OS_WIN
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_KEY_HANDLE hKey = nullptr;
    auto cleanup = [&]() { if (hKey) BCryptDestroyKey(hKey); if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0); };

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0) != 0) { cleanup(); return {}; }
    if (BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE,
                          reinterpret_cast<PUCHAR>(const_cast<wchar_t *>(BCRYPT_CHAIN_MODE_GCM)),
                          sizeof(BCRYPT_CHAIN_MODE_GCM), 0) != 0) { cleanup(); return {}; }
    if (BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0,
                                   reinterpret_cast<PUCHAR>(const_cast<char *>(key.constData())),
                                   key.size(), 0) != 0) { cleanup(); return {}; }

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = reinterpret_cast<PUCHAR>(const_cast<char *>(iv.constData()));
    authInfo.cbNonce = iv.size();
    authInfo.pbTag   = const_cast<PUCHAR>(tag);
    authInfo.cbTag   = tagLen;

    ULONG resultLen = 0;
    NTSTATUS status = BCryptDecrypt(hKey, const_cast<PUCHAR>(ct), ctLen, &authInfo, nullptr, 0,
                                    reinterpret_cast<PUCHAR>(plaintext.data()), ctLen, &resultLen, 0);
    cleanup();
    if (status != 0) return {};
    plaintext.resize(resultLen);
    return plaintext;
#else
    // Verifies the tag internally and fails the call on mismatch.
    if (CCCryptorGCMOneshotDecrypt(kCCAlgorithmAES, key.constData(), key.size(),
                                   iv.constData(), iv.size(),
                                   nullptr, 0,
                                   ct, ctLen,
                                   plaintext.data(),
                                   tag, tagLen) != kCCSuccess)
        return {};
    return plaintext;
#endif
}
