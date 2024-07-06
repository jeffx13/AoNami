#pragma once
#include <QDebug>
#include "Providers/Extractors/gogocdn.h"
#include "showprovider.h"
#include <QJsonArray>
#include "data/playinfo.h"

class AllAnime : public ShowProvider
{
private:
    QMap<QString, QString> headers = {
                                      {"authority", "api.allanime.day"},
                                      {"accept-language", "en-GB,en;q=0.9,zh-CN;q=0.8,zh;q=0.7"},
                                      {"origin", "https://allmanga.to"},
                                      {"referer", "https://allmanga.to/"},
                                      };
public:
    AllAnime() = default;
    QString name() const override { return "AllAnime"; }
    QString baseUrl = "https://allmanga.to/";
    QList<int> getAvailableTypes() const override {
        return {ShowData::ANIME};
    };
    
    QList<ShowData> search(const QString &query, int page, int type = 0) override;

    QList<ShowData> popular(int page, int type = 0) override;;

    QList<ShowData> latest(int page, int type = 0) override;

    bool loadDetails(ShowData &show, bool getPlaylist = true) const override;
    int getTotalEpisodes(const QString& link) const override {

        return 0;
    }
    QList<VideoServer> loadServers(const PlaylistItem* episode) const override {
        QJsonObject jsonResponse = NetworkClient::get(episode->link, headers).toJson();
        QList<VideoServer> servers;

        QJsonArray sourceUrls = jsonResponse["data"].toObject()["episode"].toObject()["sourceUrls"].toArray();
        for (const QJsonValue &value : sourceUrls) {
            QJsonObject server = value.toObject();
            QString name = server["sourceName"].toString();
            QString link = server["sourceUrl"].toString();
            servers.emplaceBack (name, link);

        }

        return servers;
    }

    PlayInfo extractSource(const VideoServer& server) const override {
        PlayInfo playInfo;

        QString endPoint = NetworkClient::get(baseUrl + "getVersion").toJson()["episodeIframeHead"].toString();
        auto decryptedLink = decryptSource(server.link);

        if (decryptedLink.startsWith ("/apivtwo/")) {
            decryptedLink.insert (14,".json");
            QJsonObject jsonResponse = NetworkClient::get(endPoint + decryptedLink, headers).toJson();
            QJsonArray links = jsonResponse["links"].toArray();

            for (const QJsonValue& value : links) {
                QJsonObject linkObject = value.toObject();
                if (!linkObject["dash"].toBool())
                {
                    QString source = linkObject["link"].toString();
                    playInfo.sources.emplaceBack(source);
                }

            }
        } else if (decryptedLink.contains ("streaming.php")) {
            GogoCDN gogo;
            QString source = gogo.extract (decryptedLink);
            playInfo.sources.emplaceBack(source);
        }

        return {};
    }

    QString decryptSource(const QString& input) const {
        if (input.startsWith("-")) {
            // Extract the part after the last '-'
            QString hexString = input.section('-', -1);
            QByteArray bytes;

            // Convert each pair of hex digits to a byte and append to bytes array
            for (int i = 0; i < hexString.length(); i += 2) {
                bool ok;
                QString hexByte = hexString.mid(i, 2);
                bytes.append(static_cast<char>(hexByte.toInt(&ok, 16) & 0xFF));
            }

            // XOR each byte with 56 and convert to char
            QString result;
            for (char byte : bytes) {
                result += QChar(static_cast<char>(byte ^ 56));
            }

            return result;
        } else {
            // If the input does not start with '-', return it unchanged
            return input;
        }
    }
};





