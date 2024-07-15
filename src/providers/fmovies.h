#pragma once
#include "providers/showprovider.h"
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <algorithm>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/cryptlib.h>
#include <cryptopp/arc4.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/secblock.h>

class FMovies : public ShowProvider
{
    QString tmdbURL = "https://api.themoviedb.org/3";

public:
    FMovies() = default;
    QString name() const override { return "FMovies"; }
    QString baseUrl = "https://fmovies24.to/";
    QList<int> getAvailableTypes() const override {
        return {ShowData::TVSERIES, ShowData::MOVIE};
    };
    QMap<QString, QString> vrfHeaders {
        {"Accept", "application/json, text/javascript, */*; q=0.01"},
        {"Host", QUrl(baseUrl).host()},
        {"X-Requested-With", "XMLHttpRequest"},
        };

    inline QList<ShowData> search(const QString &query, int page, int type = 0) override;
    inline QList<ShowData> popular(int page, int type = 0) override;
    inline QList<ShowData> latest(int page, int type = 0) override;

    QList<ShowData> filterSearch(const QString &filter, int page, int type);

    bool loadDetails(ShowData& show, bool getPlaylist = true) const override;
    int getTotalEpisodes(const QString &link) const override {
        return 0;
    };
    QList<VideoServer> loadServers(const PlaylistItem* episode) const override;

    PlayInfo extractSource(const VideoServer &server) const override;

    QString base64UrlSafeEncode(const QByteArray& input) const
    {
        using namespace CryptoPP;

        std::string encoded;
        StringSource ss(reinterpret_cast<const byte*>(input.data()), input.size(), true,
                        new CryptoPP::Base64Encoder(
                            new StringSink(encoded),
                            false // No padding
                            )
                        );

        // Crypto++'s Base64Encoder will not add padding by default if false is passed to it.
        // Remove padding manually if necessary (Crypto++ doesn't add it here because false was passed).

        return QString::fromStdString(encoded).replace("+", "-").replace("/", "_");
    }

    QByteArray base64UrlSafeDecode(const QString& input) const
    {
        using namespace CryptoPP;

        std::string encoded = input.toStdString();

        // Make URL safe: replace - with +, _ with /
        for (char& c : encoded) {
            if (c == '-') c = '+';
            else if (c == '_') c = '/';
        }

        // Decode Base64
        std::string decoded;
        StringSource ss(encoded, true,
                        new Base64Decoder(
                            new StringSink(decoded)
                            )
                        );

        return QByteArray::fromStdString(decoded);
    }

    QByteArray vrfShift(const QByteArray &vrf) const {
        QByteArray shiftedVrf = vrf;
        int shifts[] = {4, 3, -2, 5, 2, -4, -4, 2};

        for (int i = 0; i < shiftedVrf.size(); ++i) {
            int shift = shifts[i % 8];
            shiftedVrf[i] = shiftedVrf[i] + shift;
        }

        return shiftedVrf;
    }

    QString vrfEncrypt(const QString &input) const {
        using namespace CryptoPP;

        // RC4 Key
        byte rc4Key[] = "Ij4aiaQXgluXQRs6";
        SecByteBlock key(rc4Key, 16);

        // RC4 Encryption
        Weak::ARC4::Encryption rc4Encryption(key, key.size());
        std::string cipherText;
        StringSource ss1(input.toStdString(), true,
                         new StreamTransformationFilter(rc4Encryption,
                                                        new StringSink(cipherText)
                                                        )
                         );

        // Convert encrypted string to QByteArray
        QByteArray vrf = QByteArray::fromStdString(cipherText);

        // First Base64 encode
        QString base64Encoded = base64UrlSafeEncode(vrf);
        vrf = base64Encoded.toUtf8();

        // Second Base64 encode
        base64Encoded = base64UrlSafeEncode(vrf);
        vrf = base64Encoded.toUtf8();

        // Reverse the QByteArray
        std::reverse(vrf.begin(), vrf.end());

        // Third Base64 encode
        base64Encoded = base64UrlSafeEncode(vrf);
        vrf = base64Encoded.toUtf8();

        // Apply vrfShift
        vrf = vrfShift(vrf);

        // Convert to QString and URL encode
        QString stringVrf = QString::fromUtf8(vrf);
        QString urlEncodedVrf = QUrl::toPercentEncoding(stringVrf);

        return urlEncodedVrf;
    }

    QString vrfDecrypt(const QString& input) const
    {
        using namespace CryptoPP;

        // Base64 decode the input
        QByteArray vrf = base64UrlSafeDecode(input);

        // RC4 Key
        byte rc4Key[] = "8z5Ag5wgagfsOuhz";
        SecByteBlock key(rc4Key, 16);

        // RC4 Decryption
        Weak::ARC4::Decryption rc4Decryption(key, key.size());
        std::string decryptedText;
        StringSource ss1(reinterpret_cast<const byte*>(vrf.data()), vrf.size(), true,
                         new StreamTransformationFilter(rc4Decryption,
                                                        new StringSink(decryptedText)
                                                        )
                         );

        // Convert decrypted text to QByteArray
        vrf = QByteArray::fromStdString(decryptedText);

        // URL decode the final string
        QString stringVrf = QString::fromUtf8(vrf);
        QString urlDecodedVrf = QUrl::fromPercentEncoding(stringVrf.toUtf8());

        return urlDecodedVrf;
    }



};
