#pragma once
#include "providers/showprovider.h"
#include <QJsonArray>

class AnimePahe : public ShowProvider
{
public:
    explicit AnimePahe(QObject *parent = nullptr) : ShowProvider(parent) {}
    QString name() const override { return "AnimePahe"; }
    QString hostUrl() const override { return "https://animepahe.com/"; }

    QList<QString>     availableTypes() const override { return {"Anime"}; }
    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer server) override;

    struct Episode { double number; QString session; };

private:
    int loadShow(Client *client, ShowData &show, LoadParts parts) const override;
    // Sets reportedTotal from page 1 when countOnly, so a count needs a single request.
    QVector<Episode> fetchEpisodes(Client *client, const QString &showSession,
                                   bool countOnly, int &reportedTotal) const;

    const QMap<QString, QString> m_headers = {
        {"User-Agent",       kFirefoxUserAgent},
        {"X-Requested-With", "XMLHttpRequest"},
        {"Referer",          "https://animepahe.com/"},
        {"cookie",           "__ddg1_=;__ddg2_=;"},
    };
};
