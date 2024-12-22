#pragma once
#include <QDebug>
#include "showprovider.h"
#include <QJsonArray>
#include "player/playinfo.h"

class AllAnime : public ShowProvider
{
public:
    explicit AllAnime(QObject *parent = nullptr) : ShowProvider(parent) { setPreferredServer("Luf-mp4"); }
    QString name() const override { return "AllAnime"; }
    QString hostUrl() const override { return "https://allmanga.to/"; }

    QList<int> getAvailableTypes() const override { return {ShowData::ANIME}; }
    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer& server) const override;

private:
    QMap<QString, QString> headers = {
                                      {"authority", "api.allanime.day"},
                                      {"accept-language", "en-GB,en;q=0.9,zh-CN;q=0.8,zh;q=0.7"},
                                      {"origin", "https://allmanga.to"},
                                      {"referer", "https://allmanga.to/"},
                                      };
    QString getCoverImage(const QJsonObject &jsonResponse) const {
        QString coverUrl = jsonResponse["thumbnail"].toString();
        if (coverUrl.startsWith("https"))
            coverUrl.replace("https:/", "https://wp.youtube-anime.com");
        else
            coverUrl = "https://wp.youtube-anime.com/aln.youtube-anime.com/" + coverUrl;
        coverUrl += "?w=250";
        return coverUrl;
    }
    QString decryptSource(const QString& input) const;
    QList<ShowData> parseJsonArray(const QJsonArray &showsJsonArray, bool isPopular=false);
};





