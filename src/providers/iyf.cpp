#include "providers/iyf.h"
#include <QCryptographicHash>
#include "app/settings.h"
#include "app/exception.h"

Iyf::Iyf(QObject *parent) : ShowProvider(parent) {
    const Settings &settings = Settings::instance();
    m_expire = settings.value(QStringLiteral("iyf/auth/expire")).toString();
    m_sign   = settings.value(QStringLiteral("iyf/auth/sign")).toString();
    m_token  = settings.value(QStringLiteral("iyf/auth/token")).toString();
    m_uid    = settings.value(QStringLiteral("iyf/auth/uid")).toString();
}

QString Iyf::session() const {
    return QStringLiteral("uid=%1&expire=%2&gid=1&sign=%3&token=%4")
        .arg(m_uid, m_expire, m_sign, m_token);
}

QList<ShowData> Iyf::search(Client *client, const QString &query, int page, int /*typeIndex*/) {
    const QString tag = QString::fromUtf8(QUrl::toPercentEncoding(query.toLower()));
    const QString url = QStringLiteral("https://rankv21.iyf.tv/v3/list/briefsearch"
                                       "?tags=%1&orderby=4&page=%2&size=36&desc=1&isserial=-1&%3")
                            .arg(tag, QString::number(page), session());
    const KeyPair keys = authKeys(client);
    const auto results = client->post(url, {{"tag", tag},
                                            {"vv", sign("tags=" + tag, keys)},
                                            {"pub", keys.first}}, m_headers)
                             .toJsonObject()["data"].toObject()["info"].toArray()
                             .at(0).toObject()["result"].toArray();

    QList<ShowData> shows;
    shows.reserve(results.size());
    for (const QJsonValue &value : results) {
        const QJsonObject show = value.toObject();
        shows.emplaceBack(show["title"].toString(), show["contxt"].toString(),
                          show["imgPath"].toString(), this);
    }
    return shows;
}

QList<ShowData> Iyf::browse(Client *client, int page, bool latest, int typeIndex) {
    const int i = qBound(0, typeIndex, int(std::size(kCategoryIds)) - 1);
    const QString params = QStringLiteral("cinema=1&page=%1&size=36&orderby=%2&desc=1&cid=%3")
                               .arg(QString::number(page), latest ? "1" : "2",
                                    QLatin1String(kCategoryIds[i]));
    const auto results = callApi(client, "https://m10.iyf.tv/api/list/Search?",
                                 params + "&isserial=-1&isIndex=-1&isfree=-1")["result"].toArray();

    QList<ShowData> shows;
    shows.reserve(results.size());
    for (const QJsonValue &value : results) {
        const QJsonObject show = value.toObject();
        shows.emplaceBack(show["title"].toString(), show["key"].toString(),
                          show["image"].toString(), this, "", kShowTypes[i]);
    }
    return shows;
}

int Iyf::loadShow(Client *client, ShowData &show, LoadParts parts) const {
    const QJsonObject info = callApi(
        client, "https://m10.iyf.tv/v3/video/detail?",
        QStringLiteral("cinema=1&device=1&player=CkPlayer&tech=HLS&country=HU&lang=cns&v=1&id=%1&region=UK")
            .arg(show.link));
    if (info.isEmpty()) return 0;

    QString params = QStringLiteral("cinema=1&vid=%1&lsk=1&taxis=0&cid=%2&%3")
                         .arg(show.link, info["cid"].toString(), session());
    const KeyPair keys = authKeys(client);
    const QString vv = sign(params, keys);
    params.replace(",", "%2C");

    const auto episodes = client->get("https://m10.iyf.tv/v3/video/languagesplaylist?" + params
                                      + "&vv=" + vv + "&pub=" + keys.first)
                              .toJsonObject()["data"].toObject()["info"].toArray()
                              .at(0).toObject()["playList"].toArray();
    if (episodes.isEmpty()) return 0;
    if (parts.testFlag(CountOnly)) return episodes.size();

    if (parts.testFlag(Episodes)) {
        for (const QJsonValue &value : episodes) {
            const QJsonObject episode = value.toObject();
            show.addNumberedEpisode(0, episode["key"].toString(), episode["name"].toString());
        }
    }

    if (parts.testFlag(Details)) {
        show.description = info["contxt"].toString();
        show.status      = info["lastName"].toString();
        show.views       = QString::number(info["view"].toInt(-1));
        show.updateTime  = info["updateweekly"].toString();
        show.score       = info["score"].toString();
        show.releaseDate = info["add_date"].toString();
        show.genres.push_back(info["videoType"].toString());
    }
    return episodes.size();
}

PlayInfo Iyf::extractSource(Client *client, VideoServer server) {
    PlayInfo playInfo;
    const QJsonObject response = callApi(
        client, "https://m10.iyf.tv/v3/video/play?",
        QStringLiteral("cinema=1&id=%1&a=0&lang=none&usersign=1&region=UK&device=1&isMasterSupport=0&%2")
            .arg(server.link, session()));
    if (response.isEmpty()) return playInfo;

    for (const QJsonValue &value : response["clarity"].toArray()) {
        const QJsonObject clarity = value.toObject();
        if (clarity["path"].isNull()) continue;
        const QJsonObject path = clarity["path"].toObject();
        QString source = path["result"].toString();

        if (path["needSign"].toBool() || source.startsWith("https://hss5")) {
            const KeyPair keys = authKeys(client);
            source += QStringLiteral("&vv=%1&pub=%2").arg(sign(session(), keys), keys.first);
        }
        playInfo.videos.emplaceBack(source, "", clarity["bitrate"].toInt());
        break;
    }
    return playInfo;
}

QJsonObject Iyf::callApi(Client *client, const QString &prefixUrl, const QString &query) const {
    const KeyPair keys = authKeys(client);
    const QString url = prefixUrl + query + "&vv=" + sign(query, keys) + "&pub=" + keys.first;
    return client->get(url).toJsonObject()["data"].toObject()["info"].toArray().at(0).toObject();
}

// By value under the lock: called concurrently, so a reference to the shared static would race.
Iyf::KeyPair Iyf::authKeys(Client *client) const {
    static QMutex mutex;
    static KeyPair keys;
    QMutexLocker locker(&mutex);
    if (keys.first.isEmpty()) {
        static const QRegularExpression pattern(
            R"("publicKey":"([^"]+)\","privateKey\":\[\"([^"]+)\")");
        const auto match = pattern.match(client->get(hostUrl()).body);
        if (!match.hasMatch() || match.lastCapturedIndex() != 2)
            throw AppException("Failed to update keys", name());
        keys = {match.captured(1), match.captured(2)};
    }
    return keys;
}

QString Iyf::sign(const QString &input, const KeyPair &keys) const {
    const auto &[publicKey, privateKey] = keys;
    return QCryptographicHash::hash((publicKey + "&" + input.toLower() + "&" + privateKey).toUtf8(),
                                    QCryptographicHash::Md5).toHex();
}
