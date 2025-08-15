#pragma once
#include <QDebug>
#include "showprovider.h"
#include <QJsonArray>
#include "base/player/playinfo.h"

class AnimePahe : public ShowProvider
{
public:
    explicit AnimePahe(QObject *parent = nullptr) : ShowProvider(parent) { setPreferredServer(""); }
    QString name() const override { return "AnimePahe"; }
    QString hostUrl() const override { return "https://animepahe.ru/"; }

    QList<QString> getAvailableTypes() const override { return {"Anime"}; }
    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    int                loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer &server) override;

private:
    QString endPoint = "https://allanime.day";
    QMap<QString, QString> m_headers = {
                                      {"Origin", "https://animepahe.ru/"},
                                      {"Referer", "https://animepahe.ru/"},
                                      {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36"},
                                      };

};





