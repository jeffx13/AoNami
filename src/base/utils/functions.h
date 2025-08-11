#pragma once


#include <QString>
#include <QDebug>
// #include <cryptopp/base64.h>
// #include <cryptopp/des.h>
// #include <cryptopp/modes.h>
// #include <cryptopp/filters.h>

// #include <cryptopp/hex.h>
#include <QException>
#include <QRegularExpression>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>

#include <cryptopp/filters.h>
#include <cryptopp/md5.h>
#include <cryptopp/base64.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include "app/myexception.h"



namespace Functions{
inline void httpsIfy(QString& text){
    if (text.startsWith ("//")){
        text = "https:" + text;
    }
}
QString getHostFromUrl(const QString& url);
QString findBetween(const QString &str, const QString &a, const QString &b);
QString substringAfter(const QString &str, const QString &delimiter);
QString substringBefore(const QString &str, const QString &delimiter);
QString substring(const QString &str, const QString &delimiter1,
                        const QString &delimiter2);

inline std::string decryptAES(const std::string &ciphertextB64, const std::string &passphrase){
    using namespace CryptoPP;

    // 1. Decode the base64 ciphertext into raw bytes
    std::string rawCipher;
    {
        StringSource ss(ciphertextB64, true,
                        new Base64Decoder(
                            new StringSink(rawCipher)
                            )
                        );
    }

    // 2. Check for the "Salted__" prefix
    static const std::string SALTED_PREFIX = "Salted__";
    if (rawCipher.size() < SALTED_PREFIX.size() + 8) {
        throw std::runtime_error("Ciphertext is too short or missing salt.");
    }
    bool hasSalt = (0 == std::memcmp(rawCipher.data(), SALTED_PREFIX.data(), SALTED_PREFIX.size()));
    if (!hasSalt) {
        throw MyException("Ciphertext is missing the 'Salted__' marker.", "Decryption");
    }

    // Extract the salt (the 8 bytes right after "Salted__")
    const byte *saltPtr = reinterpret_cast<const byte*>(rawCipher.data() + SALTED_PREFIX.size());
    // The actual encrypted bytes come after that
    const std::string encryptedData = rawCipher.substr(SALTED_PREFIX.size() + 8);

    // 3. Derive the key and IV using the OpenSSL-compatible method (repeated MD5)
    // We need 32 bytes for key + 16 bytes for IV = 48 total
    const size_t KEY_LEN = 32;
    const size_t IV_LEN  = 16;
    const size_t DERIVE_LEN = KEY_LEN + IV_LEN;

    byte key[KEY_LEN], iv[IV_LEN];
    std::memset(key, 0, KEY_LEN);
    std::memset(iv, 0, IV_LEN);

    {
        // For each iteration:
        //   MD5(previous + passphrase + salt)
        // The "previous" is empty on the first iteration,
        // and is the MD5 result of the previous iteration thereafter.
        // We concatenate all MD5 results until we get 48 bytes.
        std::string md5Buffer;
        std::string result;
        std::string passSalt = passphrase + std::string(
                                   reinterpret_cast<const char*>(saltPtr), 8
                                   );

        while (result.size() < DERIVE_LEN) {
            // Build data for the MD5 input
            std::string toHash = md5Buffer + passSalt;

            // Compute MD5
            byte md5Digest[CryptoPP::Weak1::MD5::DIGESTSIZE];
            CryptoPP::Weak1::MD5 hash;
            hash.Update(reinterpret_cast<const byte*>(toHash.data()), toHash.size());
            hash.Final(md5Digest);

            // Append to cumulative
            md5Buffer.assign(reinterpret_cast<const char*>(md5Digest), sizeof(md5Digest));
            result.append(md5Buffer);
        }

        // Now extract the first 48 bytes
        std::memcpy(key, result.data(), KEY_LEN);
        std::memcpy(iv,  result.data() + KEY_LEN, IV_LEN);
    }

    // 4. Decrypt using AES-256 in CBC mode with PKCS#7 padding
    std::string plaintext;
    try
    {
        CBC_Mode<AES>::Decryption dec;
        dec.SetKeyWithIV(key, KEY_LEN, iv, IV_LEN);

        StringSource ss(encryptedData, true,
                        new StreamTransformationFilter(dec,
                                                       new StringSink(plaintext),
                                                       StreamTransformationFilter::PKCS_PADDING
                                                       )
                        );
    }
    catch (const CryptoPP::Exception &e)
    {
        // Typically "Invalid PKCS #7 block padding found" if passphrase is wrong
        throw std::runtime_error(std::string("Decryption error: ") + e.what());
    }

    return plaintext;
}


inline std::string filter(const std::string& input, const std::string& reference) {
    std::string output;
    for (char c : input) {
        if (reference.find(c) == std::string::npos) {
            output += c;
        }
    }
    return output;
}

// std::string reverseString(const std::string& str);
// std::string rc4(const std::string& key, const std::string& input);
// std::string rot13(std::string n);
// std::string base64Encode(const std::string& t,const std::string& baseTable="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
// std::string base64Decode(const std::string& input,const std::string& baseTable =  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
// std::vector<std::string> split(const std::string& str, char delimiter);
// std::vector<std::string> split(const std::string& str, const std::string& delimiter);
// std::string MD5(const std::string& str);

};



