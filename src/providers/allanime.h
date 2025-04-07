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

    QList<QString> getAvailableTypes() const override { return {"Anime"}; }
    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayItem           extractSource(Client *client, VideoServer &server) override;

private:
    QMap<QString, QString> m_headers = {
                                      {"Origin", "https://allmanga.to"},
                                      {"Referer", "https://allanime.day/"},
                                      {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/134.0.0.0 Safari/537.36"},
                                      };
    QString getCoverImage(const QJsonObject &jsonResponse) const;

    QString decryptSource(const QString& input) const;
    QList<ShowData> parseJsonArray(const QJsonArray &showsJsonArray, bool isPopular=false);

    QString msToSrtTime(double seconds) {
        int hrs = seconds / 3600;
        int mins = (int(seconds) % 3600) / 60;
        int secs = int(seconds) % 60;
        int millis = int((seconds - int(seconds)) * 1000);
        return QTime(hrs, mins, secs, millis).toString("hh:mm:ss,zzz");
    }
    QString bytesIntoHumanReadable(qint64 bytes) {
        const qint64 kilobyte = 1000;
        const qint64 megabyte = kilobyte * 1000;
        const qint64 gigabyte = megabyte * 1000;
        const qint64 terabyte = gigabyte * 1000;

        if (bytes >= 0 && bytes < kilobyte) {
            return QString::number(bytes) + " b/s";
        } else if (bytes < megabyte) {
            return QString::number(bytes / kilobyte) + " kb/s";
        } else if (bytes < gigabyte) {
            return QString::number(bytes / megabyte) + " mb/s";
        } else if (bytes < terabyte) {
            return QString::number(bytes / gigabyte) + " gb/s";
        } else {
            return QString::number(bytes / terabyte) + " tb/s";
        }
    }
};





