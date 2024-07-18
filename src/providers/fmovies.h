#pragma once
#include "providers/showprovider.h"
#include <QByteArray>
#include <QString>
#include <QUrl>
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

    QList<ShowData> search(const QString &query, int page, int type = 0) override;
    QList<ShowData> popular(int page, int type = 0) override;
    QList<ShowData> latest(int page, int type = 0) override;

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

    QByteArray base64UrlSafeDecode(const QString& input) const;

    QByteArray vrfShift(const QByteArray &vrf) const {
        QByteArray shiftedVrf = vrf;
        int shifts[] = {4, 3, -2, 5, 2, -4, -4, 2};

        for (int i = 0; i < shiftedVrf.size(); ++i) {
            int shift = shifts[i % 8];
            shiftedVrf[i] = shiftedVrf[i] + shift;
        }

        return shiftedVrf;
    }

    QString vrfEncrypt(const QString &input) const;

    QString vrfDecrypt(const QString& input) const;



};
