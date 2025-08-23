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

    
    // // ckey/encryption version number. Possible values are: 8.5, 8.1, 9.1
    // QMap<QString, QString> encryptVerToAppVer {
    //     { "8.1", "3.5.57" },
    //     { "9.1", "3.5.57" },
    //     { "8.5", "1.27.3" }
    // };

    // QMap<QString, QString> _VQQ_FMT2DEFN_MAP {
    //     { "10209", "fhd" }, // 4K
    //     { "10201", "shd" }, // 2K
    //     { "10212", "hd" },  // HD
    //     { "10203", "sd" },  // SD
    //     { "321004", "fhd" }, // 4K
    //     { "321003", "shd" }, // 2K
    //     { "321002", "hd" },  // HD
    //     { "321001", "sd" },  // SD
    //     { "320090", "hd" },  // HD
    //     { "320089", "sd" }   // SD
    // };

    // QString encryptVer = "8.5";
    // QString ckey_js = QString("vqq_ckey-%1.js").arg(encryptVer);
    // QString jsFile = QCoreApplication::applicationDirPath() + "/" + ckey_js;
    // QString appVer = encryptVerToAppVer[encryptVer];
    // bool probeMode= false;
    // QString m_loginToken;
    // QString vinfoparams = QString("otype=ojson&isHLS=1&charge=0&fhdswitch=0&show1080p=1&defnpayver=7&sdtfrom=v1010&host=v.qq.com&vid=&defn=%1&platform=10201&appVer=%2").arg(definition, appVer);
    // QString apiUrl = "https://vd.l.qq.com/proxyhttp";
    // QStringList getCkey(QStringList &args) const;
};


