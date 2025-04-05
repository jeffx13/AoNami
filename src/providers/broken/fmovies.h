// #pragma once
// #include "providers/showprovider.h"
// #include <QByteArray>
// #include <QString>
// #include <QUrl>
// #define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
// #include <cryptopp/cryptlib.h>
// #include <cryptopp/arc4.h>
// #include <cryptopp/base64.h>
// #include <cryptopp/filters.h>
// #include <cryptopp/secblock.h>

// class FMovies : public ShowProvider
// {
//     QString tmdbURL = "https://api.themoviedb.org/3";

// public:
//     FMovies() = default;
//     QString name() const override { return "FMovies"; }
//     QString hostUrl = "https://flix2day.to/";
//     QList<QString> getAvailableTypes() const override {
//         return {ShowData::TVSERIES, ShowData::MOVIE};
//     };
//     QMap<QString, QString> vrfHeaders {
//         {"Accept", "application/json, text/javascript, */*; q=0.01"},
//         {"Host", QUrl(hostUrl).host()},
//         {"X-Requested-With", "XMLHttpRequest"},
//         };

//     QList<ShowData> search(Client *client, const QString &query, int page, int type = 0) override;
//     QList<ShowData> popular(Client *client, int page, int type = 0) override;
//     QList<ShowData> latest(Client *client, int page, int type = 0) override;

//     QList<ShowData> filterSearch(Client *client, const QString &filter, int page, int type);

//     int loadDetails(Client *client, ShowData& show, bool loadInfo, bool getPlaylist = true) const override;

//     QList<VideoServer> loadServers(Client *client, const PlaylistItem* episode) const override;

//     PlayItem extractSource(Client *client, VideoServer &server) override;

//     QString base64UrlSafeEncode(const QByteArray& input) const;

//     QByteArray base64UrlSafeDecode(const QString& input) const;

//     QByteArray vrfShift(const QByteArray &vrf) const {
//         QByteArray shiftedVrf = vrf;
//         int shifts[] = {4, 3, -2, 5, 2, -4, -4, 2};

//         for (int i = 0; i < shiftedVrf.size(); ++i) {
//             int shift = shifts[i % 8];
//             shiftedVrf[i] = shiftedVrf[i] + shift;
//         }

//         return shiftedVrf;
//     }


//     QString vrfDecrypt(const QString& input) const;

//     // Function to map characters from one set to another
//     QString mapCharacters(const QString &text, const QString &fromChars, const QString &toChars) const{
//         QMap<QChar, QChar> charMap;
//         int length = fromChars.length();

//         // Create a mapping from `fromChars` to `toChars`
//         for (int i = 0; i < length; i++) {
//             charMap[fromChars[i]] = toChars[i];
//         }

//         // Replace characters in `text` based on the mapping
//         QString result;
//         for (const QChar &ch : text) {
//             result += charMap.contains(ch) ? charMap[ch] : ch;
//         }

//         return result;
//     }

//     // Function to reverse a string
//     QString reverseString(QString &text)const {
//         std::reverse(text.begin(), text.end());
//         return text;
//     }

//     // Function to perform RC4 encryption using Crypto++'s ARC4 implementation
//     QByteArray rc4Encrypt(const QString &key, const QString &text) const {
//         QByteArray keyBytes = key.toUtf8();
//         QByteArray textBytes = text.toUtf8();
//         using namespace CryptoPP;
//         Weak::ARC4::Encryption rc4(reinterpret_cast<const byte*>(keyBytes.data()), keyBytes.size());

//         QByteArray encryptedText;
//         encryptedText.resize(textBytes.size());
//         rc4.ProcessData(reinterpret_cast<byte*>(encryptedText.data()), reinterpret_cast<const byte*>(textBytes.data()), textBytes.size());

//         // Convert the encrypted byte array back to a QString
//         return encryptedText;
//     }

//     // Function to perform base64 URL encoding
//     QString base64UrlEncode(const QString &input)const {
//         QByteArray byteArray = QByteArray::fromHex(input.toUtf8());
//         QByteArray base64 = byteArray.toBase64();
//         return QString::fromUtf8(base64).replace("/", "_").replace("+", "-").replace("=", "");
//     }

//     // Function to perform base64 URL decoding
//     QString base64UrlDecode(const QString &input)const {
//         QString base64 = input;
//         base64 = base64.replace("_", "/").replace("-", "+");
//         int paddingNeeded = (4 - (base64.length() % 4)) % 4;
//         base64.append(QString(paddingNeeded, '='));
//         QByteArray byteArray = QByteArray::fromBase64(base64.toUtf8());
//         return QString::fromUtf8(byteArray);
//     }

//     // The main function equivalent to the JavaScript `R` function
//     QString vrfEncrypt(QString input) const {
//         input = mapCharacters(input, "5j6Ak1GJaTy8XoC", "56kC8jyGoXTAa1J");
//         // qDebug() << input;
//         input = reverseString(input);
//         // qDebug() << input;
//         auto rc4Encrypted = rc4Encrypt("hAGMmLFnoa0", input);
//         // qDebug() << rc4Encrypted;

//         input = base64UrlSafeEncode(rc4Encrypted);
//         // qDebug() << input;

//         input = mapCharacters(input, "PUoVzgdK5FLZt", "FVogUPtKzdZL5");
//         // qDebug() << input;
//         input = reverseString(input);
//         // qDebug() << input;
//         rc4Encrypted = rc4Encrypt("oUHxby23izOI5", input);
//         // qDebug() << input;
//         input = base64UrlSafeEncode(rc4Encrypted);
//         // qDebug() << input;

//         input = mapCharacters(input, "PEQmieNvWhrOX", "OEehvmXQrWiPN");
//         // qDebug() << input;
//         input = reverseString(input);
//         // qDebug() << input;
//         rc4Encrypted = rc4Encrypt("tX6D4K8mPrq3V", input);
//         // qDebug() << input;
//         input = base64UrlSafeEncode(rc4Encrypted);
//         // qDebug() << input;
//         return base64UrlSafeEncode(input.toUtf8());
//     }


// };
