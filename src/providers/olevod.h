#pragma once
#include "providers/showprovider.h"

class Olevod : public ShowProvider {
public:
    explicit Olevod(QObject *parent = nullptr) : ShowProvider(parent) {}
    QString name() const override { return "OleVod"; }
    QString hostUrl() const override { return "https://www.olevod.com/"; }

    QList<QString>     getAvailableTypes() const override { return {"动漫", "电影", "连续剧", "综艺"}; }
    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer server) override;

private:
    int loadShow(Client *client, ShowData &show, LoadParts parts) const override;

    static QString signedUrl(const QString &path);
    static QString vv(qint64 unixSeconds);
    QList<ShowData> listing(Client *client, int page, int typeIndex, const QString &sort);
    QList<ShowData> parseList(const QJsonArray &items);

    static constexpr const char *kApi = "https://api.olelive.com";
    static constexpr const char *kStatic = "https://static.olelive.com/";
    static constexpr int kTypeIds[] = {4, 1, 2, 3};
    static constexpr int kPageSize = 48;

    QMap<QString, QString> m_headers = {
        {"User-Agent", kFirefoxUserAgent},
        {"Origin",     "https://www.olevod.com"},
        {"Referer",    "https://www.olevod.com/"},
    };
};
