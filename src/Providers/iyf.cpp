#include "iyf.h"

IyfProvider::IyfProvider() {
    // if (publicKey.isEmpty()) updateKeys();
    // updateKeys();
    auto keyUpdator = QtConcurrent::run (&IyfProvider::updateKeys, this);
}

QList<ShowData> IyfProvider::search(const QString &query, int page, int type) {
    QList<ShowData> shows;
    QString tag = QUrl::toPercentEncoding (query.toLower());
    QString url = QString("https://rankv21.iyf.tv/v3/list/briefsearch?tags=%1&orderby=4&page=%2&size=36&desc=1&isserial=-1&uid=%3&expire=%4&gid=0&sign=%5&token=%6")
                      .arg(tag, QString::number (page), uid, expire, sign, token);
    auto resultsJson = NetworkClient::post(url ,headers, { {"tag", tag}, {"vv", hash("tags=" + tag)}, {"pub", publicKey} })
                            .toJson()["data"].toObject()["info"].toArray().at (0).toObject()["result"].toArray();

    for (const QJsonValue &value : resultsJson) {
        QJsonObject showJson = value.toObject();
        QString title = showJson["title"].toString();
        QString link = showJson["contxt"].toString();
        QString coverUrl = showJson["imgPath"].toString();
        shows.emplaceBack(title, link, coverUrl, this);
    }
    return shows;
}

QList<ShowData> IyfProvider::filterSearch(int page, bool latest, int type) {
    QList<ShowData> shows;
    QString orderBy = latest ? "1" : "2";
    QString params = QString("cinema=1&page=%1&size=36&orderby=%2&desc=1&cid=%3%4")
                         .arg(QString::number (page), orderBy, cid[type], latest ? "" : "&year=今年");
    auto resultsJson = invokeAPI("https://m10.iyf.tv/api/list/Search?", params + "&isserial=-1&isIndex=-1&isfree=-1")["result"].toArray();

    for (const QJsonValue &value : resultsJson) {
        QJsonObject showJson = value.toObject();
        QString coverUrl = showJson["image"].toString();

        QString title = showJson["title"].toString();
        QString link = showJson["key"].toString();
        shows.emplaceBack(title, link, coverUrl, this);
    }
    return shows;
}

bool IyfProvider::loadDetails(ShowData &show, bool getPlaylist) const {
    QString params = QString("cinema=1&device=1&player=CkPlayer&tech=HLS&country=HU&lang=cns&v=1&id=%1&region=UK").arg (show.link);
    auto infoJson = invokeAPI("https://m10.iyf.tv/v3/video/detail?", params);
    if (infoJson.isEmpty()) return false;

    show.description =  infoJson["contxt"].toString();
    show.status = infoJson["lastName"].toString();
    show.views =  QString::number(infoJson["view"].toInt (-1));
    show.updateTime = infoJson["updateweekly"].toString();
    show.score = infoJson["score"].toString();
    show.releaseDate = infoJson["add_date"].toString();
    show.genres.push_back (infoJson["videoType"].toString());

    if (!getPlaylist) return true;

    QString cid = infoJson["cid"].toString();
    params = QString("cinema=1&vid=%1&lsk=1&taxis=0&cid=%2&uid=%3&expire=%4&gid=4&sign=%5&token=%6")
                 .arg(show.link, cid, uid, expire, sign, token);
    auto vv = hash(params);
    params.replace (",", "%2C");
    QString url = "https://m10.iyf.tv/v3/video/languagesplaylist?" + params + "&vv=" + vv + "&pub=" + publicKey;
    auto playlistJson = NetworkClient::get (url).toJson()["data"].toObject()["info"].toArray().at (0).toObject()["playList"].toArray();
    if (playlistJson.isEmpty ()) return false;

    bool ok;
    float number = -1;
    QString title;
    QString link;
    for (const QJsonValue &value : playlistJson) {
        QJsonObject episodeJson = value.toObject();
        title = episodeJson["name"].toString();
        number = -1;
        float intTitle = title.toFloat (&ok);
        if (ok) {
            number = intTitle;
            title = "";
        }
        link = episodeJson["key"].toString();
        show.addEpisode(number, link, title);
    }

    return true;
}

PlayInfo IyfProvider::extractSource(const VideoServer &server) const {
    PlayInfo playInfo;

    QString params = QString("cinema=1&id=%1&a=0&lang=none&usersign=1&region=UK&device=1&isMasterSupport=1&uid=%2&expire=%3&gid=0&sign=%4&token=%5")
                         .arg (server.link,uid, expire, sign, token);

    auto clarities = invokeAPI("https://m10.iyf.tv/v3/video/play?", params)["clarity"].toArray();
    for (const QJsonValue &value : clarities) {
        auto clarity = value.toObject();
        auto path = clarity["path"];
        if (!path.isNull()) {
            QString source = path.toObject()["result"].toString();
            params = QString("uid=%1&expire=%2&gid=0&sign=%3&token=%4")
                         .arg (uid, expire, sign, token);
            source += "?" + params + "&vv=" + hash(params) + "&pub=" + publicKey;
            playInfo.sources.emplaceBack (source);
        }
    }
    return playInfo;
}


