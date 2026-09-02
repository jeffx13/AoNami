#pragma once
#include <QByteArray>

// AES for the provider extractors, over Windows CNG (bcrypt).
namespace Aes {

// AES-CTR. `counter` is the 16-byte initial block and increments big-endian.
QByteArray ctr(const QByteArray &key, QByteArray counter, const QByteArray &input);

// ctWithTag is ciphertext + 16-byte tag (WebCrypto layout); empty on tag mismatch.
QByteArray gcmDecrypt(const QByteArray &key, const QByteArray &iv, const QByteArray &ctWithTag);

}
