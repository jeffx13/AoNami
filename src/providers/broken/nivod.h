// #pragma once
// #include "showprovider.h"
// #include <QJsonArray>

// class Nivod: public ShowProvider
// {
// public:
//     Nivod(){};
//     QString name() const override {return "泥巴影院";}
//     QString hostUrl = "https://www.nivod4.tv/";
//     inline QList<QString> getAvailableTypes() const override {
//         return {ShowData::ANIME, ShowData::MOVIE, ShowData::TVSERIES, ShowData::VARIETY, ShowData::DOCUMENTARY};
//     };

//     QList<ShowData> search(Client *client, const QString &query, int page, int type) override;
//     inline QList<ShowData> popular(Client *client, int page, int typeIndex) override { return filterSearch(page, "1", type); };
//     inline QList<ShowData> latest(Client *client, int page, int typeIndex) override { return filterSearch(page, "4", type); };

//     bool loadDetails(Client *client, ShowData &show, bool loadInfo, bool getPlaylist = true) const override;
//     inline QList<VideoServer> loadServers(Client *client, const PlaylistItem *episode) const override { return {VideoServer{"Default", episode->link}}; };
//     inline int getTotalEpisodes(const QString &link) const override { return getInfoJson(link)["plays"].toArray().size(); }
//     PlayItem extractSource(VideoServer& server) const override;

// private:
//     QJsonObject getInfoJson(const QString& link) const;
//     QList<ShowData> parseShows(const QJsonArray& showList);
//     QList<ShowData> filterSearch(int page, const QString& sortBy, int type, const QString& regionId = "0",
//                                  const QString& langId = "0", const QString& yearRange = " ");
//     QJsonObject invokeAPI(const QString& url, const QMap<QString, QString>& data) const;
//     std::string decryptedByDES(const std::string &input) const;

// private:
//     QMap<QString, QString> m_headers{ {"referer", "https://www.nivod5.tv"} };
//     // const QString _HOST_CONFIG_KEY = "2x_Give_it_a_shot";
//     const QString _QUERY_PREFIX = "__QUERY::";
//     const QString _BODY_PREFIX = "__BODY::";
//     const QString _SECRET_PREFIX = "__KEY::";
//     const QString _mts = "1712541606828";
//     const QString _oid = "fc3d2a97c1aae488ac22c2072b4a6abe923be124e8f35937";
//     const QMap<QString, QString> queryMap =
//         {
//             {"_ts", _mts},
//             {"app_version", "1.0"},
//             {"device_code", "web"},
//             {"market_id", "web_nivod"},
//             {"oid", _oid},
//             {"platform", "3"},
//             {"versioncode", "1"}
//         };

//     QString MD5(const QString &str) const;
// };

// // extern "C" __declspec(dllexport) Nivod* createPlugin() {
// //     return new Nivod();
// // }

