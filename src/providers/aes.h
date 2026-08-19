#pragma once
#include <QByteArray>

// AES for the provider extractors. Windows uses CNG (bcrypt), everything else CommonCrypto.
namespace Aes {

// AES-CTR. `counter` is the 16-byte initial block and increments big-endian.
QByteArray ctr(const QByteArray &key, QByteArray counter, const QByteArray &input);

// AES-GCM. `ctWithTag` is ciphertext followed by the 16-byte tag (WebCrypto layout).
// Returns empty on tag mismatch.
QByteArray gcmDecrypt(const QByteArray &key, const QByteArray &iv, const QByteArray &ctWithTag);

}
