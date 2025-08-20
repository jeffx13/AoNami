#include "iyf.h"
#include "app/config.h"

IyfProvider::IyfProvider(QObject *parent) : ShowProvider(parent) {
    auto config = Config::get();
    if (!config.contains("iyf_auth"))
        return;

    auto auth = config["iyf_auth"].toObject();
    expire = auth["expire"].toString();
    sign = auth["sign"].toString();
    token = auth["token"].toString();
    uid = auth["uid"].toString();
}

QList<ShowData> IyfProvider::search(Client *client, const QString &query, int page, int type) {
    QList<ShowData> shows;
    QString tag = QUrl::toPercentEncoding (query.toLower());
    QString url = QString("https://rankv21.iyf.tv/v3/list/briefsearch?tags=%1&orderby=4&page=%2&size=36&desc=1&isserial=-1&uid=%3&expire=%4&gid=0&sign=%5&token=%6")
                      .arg(tag, QString::number (page), uid, expire, sign, token);
    auto &keys = getKeys(client);
    auto resultsJson = client->post(url, { {"tag", tag}, {"vv", hash("tags=" + tag, keys)}, {"pub", keys.first} }, headers)
                           .toJsonObject()["data"].toObject()["info"].toArray().at (0).toObject()["result"].toArray();
    Q_FOREACH(const QJsonValue &value, resultsJson) {
        QJsonObject showJson = value.toObject();
        QString title = showJson["title"].toString();
        QString link = showJson["contxt"].toString();
        QString coverUrl = showJson["imgPath"].toString();
        shows.emplaceBack(title, link, coverUrl, this);
    }
    return shows;
}

QList<ShowData> IyfProvider::filterSearch(Client *client, int page, bool latest, int typeIndex) {
    QList<ShowData> shows;

    ShowData::ShowType type = m_typeIndexToType[typeIndex];
    QString orderBy = latest ? "1" : "2";
    QString params = QString("cinema=1&page=%1&size=36&orderby=%2&desc=1&cid=%3%4")
                         .arg(QString::number (page), orderBy, cid[typeIndex], latest ? "" : "");//&year=今年
    auto resultsJson = invokeAPI(client, "https://m10.iyf.tv/api/list/Search?", params + "&isserial=-1&isIndex=-1&isfree=-1");
    Q_FOREACH(const QJsonValue &value, resultsJson["result"].toArray()) {
        QJsonObject showJson = value.toObject();
        QString coverUrl = showJson["image"].toString();
        QString title = showJson["title"].toString();
        QString link = showJson["key"].toString();

        shows.emplaceBack(title, link, coverUrl, this, "", type);
    }
    return shows;
}

int IyfProvider::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
    QString params = QString("cinema=1&device=1&player=CkPlayer&tech=HLS&country=HU&lang=cns&v=1&id=%1&region=UK").arg (show.link);
    auto infoJson = invokeAPI(client, "https://m10.iyf.tv/v3/video/detail?", params);
    if (infoJson.isEmpty()) return false;


    QString cid = infoJson["cid"].toString();
    params = QString("cinema=1&vid=%1&lsk=1&taxis=0&cid=%2&uid=%3&expire=%4&gid=0&sign=%5&token=%6")
                 .arg(show.link, cid, uid, expire, sign, token);
    auto keys = getKeys(client);
    auto vv = hash(params, keys);
    params.replace (",", "%2C");

    QString url = "https://m10.iyf.tv/v3/video/languagesplaylist?" + params + "&vv=" + vv + "&pub=" + keys.first;
    auto playlistJson = client->get (url).toJsonObject()["data"].toObject()["info"].toArray().at (0).toObject()["playList"].toArray();

    if (playlistJson.isEmpty ()) return 0;
    if (getEpisodeCountOnly) return playlistJson.size();
    if (getPlaylist) {
        Q_FOREACH(const QJsonValue &value, playlistJson) {
            QJsonObject episodeJson = value.toObject();
            QString title = episodeJson["name"].toString();
            float number = resolveTitleNumber(title);
            if (number != -1) {
                title = "";
            }
            QString link = episodeJson["key"].toString();
            show.addEpisode(0, number, link, title);
        }
    }
    if (!getInfo) return playlistJson.size();

    show.description =  infoJson["contxt"].toString();
    show.status = infoJson["lastName"].toString();
    show.views =  QString::number(infoJson["view"].toInt (-1));
    show.updateTime = infoJson["updateweekly"].toString();
    show.score = infoJson["score"].toString();
    show.releaseDate = infoJson["add_date"].toString();
    show.genres.push_back (infoJson["videoType"].toString());



    return playlistJson.size();
}

PlayInfo IyfProvider::extractSource(Client *client, VideoServer &server) {

    PlayInfo playItem;
    QString query = QString("cinema=1&id=%1&a=0&lang=none&usersign=1&region=UK&device=1&isMasterSupport=1&sharpness=1080&uid=%2&expire=%3&gid=0&sign=%4&token=%5")
                        .arg (server.link, uid, expire, sign, token);

    auto response = invokeAPI(client, "https://m10.iyf.tv/v3/video/play?", query);
    if (response.isEmpty()) return playItem;
    auto clarities = response["clarity"].toArray();
    Q_FOREACH(const QJsonValue &value, clarities) {
        auto clarity = value.toObject();
        if (clarity["path"].isNull()) continue;
        // auto title = clarity["title"].toString();
        // auto description = clarity["description"].toString();
        auto bitrate = clarity["bitrate"].toInt();
        QJsonObject path = clarity["path"].toObject();
        QString source = path["result"].toString();
        // QString label = QString("%2 (%1)").arg(title, description);

        if (path["needSign"].toBool()) {
            auto &keys = getKeys(client);
            auto s = QString("uid=%1&expire=%2&gid=0&sign=%3&token=%4").arg(uid, expire, sign, token);
            source += QString("?%1&vv=%2&pub=%3").arg(s, hash(s, keys), keys.first);
        }

        playItem.videos.emplaceBack(source, "", bitrate);
        break;
    }
    return playItem;
}

QJsonObject IyfProvider::invokeAPI(Client *client, const QString &prefixUrl, const QString &query) const {
    auto &keys = getKeys(client);
    auto url = prefixUrl + query + "&vv=" + hash(query, keys) + "&pub=" + keys.first;

    return client->get(url).toJsonObject()["data"].toObject()["info"].toArray().at(0).toObject();
}

QPair<QString, QString> &IyfProvider::getKeys(Client *client, bool update) const {
    static QPair<QString, QString> keys;
    if (keys.first.isEmpty() || update) {

        auto response = client->get(hostUrl());
        static QRegularExpression pattern(R"("publicKey":"([^"]+)\","privateKey\":\[\"([^"]+)\")");
        QRegularExpressionMatch match = pattern.match(response.body);
        // Perform the search
        if (!match.hasMatch() || match.lastCapturedIndex() != 2)
            throw MyException("Failed to update keys", name());
        keys = {match.captured(1), match.captured(2)};
    }
    return keys;
}

QString IyfProvider::hash(const QString &input, const QPair<QString, QString> &keys) const {
    auto &[publicKey, privateKey] = keys;
    auto toHash = publicKey + "&"  + input.toLower() + "&"  + privateKey;
    QByteArray hash = QCryptographicHash::hash(toHash.toUtf8(), QCryptographicHash::Md5);
    return hash.toHex();
}


