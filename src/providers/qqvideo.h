#pragma once
#include "showprovider.h"

#include <QCoreApplication>
#include <QProcess>



class QQVideo : public ShowProvider
{
public:
    explicit QQVideo(QObject *parent = nullptr);;

    QString name() const override { return "腾讯视频"; }
    QString hostUrl() const override { return "https://v.qq.com"; }
    QList<QString> getAvailableTypes() const override {
        return {"动漫", "电影", "电视剧", "综艺", "纪录片", "少儿", "短剧"};
    };

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
    QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer &server) override;
private:
    QList<ShowData>    filterSearch (Client *client, int sortBy, int page, int type);
    int                loadShow  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
    QString            m_loginToken;
    QList<int> channelIds = {
        100119, // 动漫 (index 0)
        100173, // 电影 (index 1)
        100113, // 电视剧 (index 2)
        100109, // 综艺 (index 3)
        100105, // 纪录片 (index 4)
        100150, // 少儿 (index 5)
        110755, // 短剧 (index 6)
    };
    QMap<QString, QString> headers {
        {"accept", "application/json"},
        {"accept-language", "en-GB,en-US;q=0.9,en;q=0.8"},
        {"content-type", "application/json"},
        {"origin", "https://v.qq.com"},
        {"referer", "https://v.qq.com/"},
        {"user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/139.0.0.0 Safari/537.36"}
    };

    QList<ShowData::ShowType> m_typeIndexToType = {
        ShowData::ANIME,       // 动漫
        ShowData::MOVIE,       // 电影
        ShowData::TVSERIES,    // 电视剧
        ShowData::VARIETY,     // 综艺
        ShowData::DOCUMENTARY, // 纪录片
        ShowData::NONE,        // 少儿
        ShowData::NONE         // 短剧
    };
    QStringList getCkey(QStringList &args) const;

};


