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
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer &server) override;

private:
    int                loadShow  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
    QMap<QString, QString> m_headers = {
                                      {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:143.0) Gecko/20100101 Firefox/143.0"},
                                      {"X-Requested-With", "XMLHttpRequest"},
                                      {"Referer", "https://animepahe.ru/"},
                                      {"cookie", "__ddg1_=;__ddg2_=;"},
                                      };

    

};





