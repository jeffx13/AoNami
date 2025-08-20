#include "animepahe.h"
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QJSEngine>
#include <QJSValue>
#include "base/utils/functions.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <cmath>

// Minimal DDos-Guard handling tailored for AnimePahe requests
// static Client::Response ap_get_with_ddg(Client *client, const QString &url, const QMap<QString, QString> &baseHeaders) {
// 	// First attempt
// 	Client::Response resp = client->get(url, baseHeaders);
// 	QString serverHeader = resp.headers.value("Server", resp.headers.value("server"));
// 	if (!(resp.code == 403 && serverHeader.contains("ddos-guard", Qt::CaseInsensitive))) {
// 		return resp;
// 	}
// 	// Try to obtain __ddg2_ cookie
// 	Client::Response jsResp = client->get("https://check.ddos-guard.net/check.js", baseHeaders);
// 	QString js = jsResp.body;
// 	int a = js.indexOf('\'');
// 	int b = js.indexOf('\'', a + 1);
// 	if (a < 0 || b <= a) {
// 		return resp; // give up
// 	}
// 	QUrl u(url);
// 	QString wellKnown = js.mid(a + 1, b - a - 1);
// 	QString checkUrl = u.scheme() + "://" + u.host() + wellKnown;
// 	Client::Response ck = client->get(checkUrl, baseHeaders);
// 	QString setCookie = ck.headers.value("set-cookie");
// 	if (setCookie.isEmpty()) {
// 		return resp; // still blocked
// 	}
// 	QString cookieNv = setCookie.split(';').first();
// 	QMap<QString, QString> headers = baseHeaders;
// 	QString existingCookie = headers.value("cookie");
// 	if (!existingCookie.isEmpty()) headers["cookie"] = existingCookie + "; " + cookieNv; else headers["cookie"] = cookieNv;
// 	return client->get(url, headers);
// }



QList<ShowData> AnimePahe::search(Client *client, const QString &query, int page, int type) {
    Q_UNUSED(type);
	QList<ShowData> shows;
    if (page > 1) return shows;
	if (query.trimmed().isEmpty()) return shows;
	QString url = hostUrl() + "api?m=search&q=" + QUrl::toPercentEncoding(query);
    QJsonObject root = client->get(url, m_headers).toJsonObject();
	QJsonArray items = root.value("data").toArray();
	Q_FOREACH (const QJsonValue &v, items) {
		QJsonObject o = v.toObject();
		QString title = o.value("title").toString();
		QString session = o.value("session").toString();
		QString poster = o.value("poster").toString();
		if (title.isEmpty() || session.isEmpty()) continue;
		shows.emplaceBack(title, session, poster, this, "", ShowData::ANIME);
	}
	return shows;
}

QList<ShowData> AnimePahe::popular(Client *client, int page, int typeIndex) {
    return latest(client, page, typeIndex);
}

QList<ShowData> AnimePahe::latest(Client *client, int page, int typeIndex) {
    Q_UNUSED(typeIndex);
    QList<ShowData> shows;
    QString url = hostUrl() + "api?m=airing&page=" + QString::number(page);
    QJsonObject root = client->get(url, m_headers).toJsonObject();
    QJsonArray items;
    if (root.contains("data")) items = root.value("data").toArray();
    else if (root.contains("items")) items = root.value("items").toArray();
    QSet<QString> seen;
    Q_FOREACH (const QJsonValue &v, items) {
        QJsonObject o = v.toObject();
        QString title = o.value("anime_title").toString();
        // QString animeId = o.value("anime_id").toString();
        QString animeSession = o.value("anime_session").toString();
        QString linkValue = animeSession;
        if (title.isEmpty() || linkValue.isEmpty()) continue;
        if (seen.contains(linkValue)) continue;
        seen.insert(linkValue);
        QString snapshot = o.value("snapshot").toString();
        QString fansub = o.value("fansub").toString();
        shows.emplaceBack(title, linkValue, snapshot, this, fansub, ShowData::ANIME);
    }
    return shows;
}

int AnimePahe::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
	// Load episodes via paginated API (request desc then sort asc locally)
	QVector<QPair<double, QString>> allEpisodes;
	{
		int page = 1;
		while (true) {
            QString url = hostUrl() + QString("api?m=release&id=%1&sort=episode_desc&page=%2").arg(show.link).arg(page);
            QJsonObject root = client->get(url, m_headers).toJsonObject();
            if (root.isEmpty()) break;
			QJsonArray items;
			if (root.contains("data")) items = root.value("data").toArray();
			else if (root.contains("items")) items = root.value("items").toArray();
			QVector<QPair<double, QString>> eps;
			eps.reserve(items.size());
			Q_FOREACH(const QJsonValue &v, items) {
				QJsonObject ep = v.toObject();
				QString epSession = ep.value("session").toString();
				if (epSession.isEmpty()) continue;
				double epNum = ep.value("episode").toDouble(-1.0);
				if (epNum < 0 && ep.contains("episode2")) epNum = ep.value("episode2").toDouble(-1.0);
				eps.append({epNum, epSession});
			}
			
			allEpisodes += eps;
			int currentPage = root.value("current_page").toInt(root.value("currentPage").toInt(1));
			int lastPage = root.value("last_page").toInt(root.value("lastPage").toInt(currentPage));
			if (currentPage >= lastPage) break;
			page = currentPage + 1;
			if (client->isCancelled()) break;
		}
	}
	int totalCount = allEpisodes.size();
	if (getEpisodeCountOnly && !getPlaylist && !getInfo) return totalCount;

	if (getPlaylist) {
		std::sort(allEpisodes.begin(), allEpisodes.end(), [](const QPair<double, QString> &a, const QPair<double, QString> &b){
			return a.first < b.first;
		});
		Q_FOREACH (const auto &p, allEpisodes) {
            QString link = QString("/play/%1/%2").arg(show.link, p.second);
			show.addEpisode(0, (float)p.first, link, QString());
		}
	}

	if (getInfo) {
        QString pageUrl = hostUrl() + "anime/" + show.link;
        auto doc = client->get(pageUrl, m_headers).toSoup();
		if (doc) {
			// description
			auto descNode = doc.selectFirst("//div[contains(@class,'anime-summary')]");
            if (descNode) show.description = descNode.text().trimmed();
			// cover
			auto coverNode = doc.selectFirst("//div[contains(@class,'anime-poster')]//img");
			if (coverNode) show.coverUrl = coverNode.attr("data-src");
			if (show.coverUrl.isEmpty() && coverNode) show.coverUrl = coverNode.attr("src");
			// status
			auto statusNode = doc.selectFirst("//div[contains(@class,'anime-info')]//p[strong[contains(text(),'Status:')]]//a");
			if (statusNode) show.status = statusNode.text().trimmed();
			// genres
			auto genreNodes = doc.select("//div[contains(@class,'anime-genre')]//li");
			Q_FOREACH (const auto &g, genreNodes) show.genres.push_back(g.text().trimmed());

			// additional info from right-side info panel
			auto infoPanel = doc.selectFirst("//div[contains(@class,'col-sm-4') and contains(@class,'anime-info')]");
			QStringList extraLines;
			if (infoPanel) {
				auto syn = infoPanel.selectFirst(".//p[strong[contains(text(),'Synonyms')]]");
                if (syn) extraLines << syn.text().trimmed();
                // auto jap = infoPanel.selectFirst(".//p[strong[contains(text(),'Japanese')]]");
                // if (jap) extraLines << jap.text().simplified();
				auto typeP = infoPanel.selectFirst(".//p[strong[contains(text(),'Type')]]");
                if (typeP) extraLines << typeP.text().trimmed();
				auto epsP = infoPanel.selectFirst(".//p[strong[contains(text(),'Episodes')]]");
                if (epsP) extraLines << epsP.text().trimmed();
				auto durP = infoPanel.selectFirst(".//p[strong[contains(text(),'Duration')]]");
                if (durP) extraLines << durP.text().trimmed();
				auto airedP = infoPanel.selectFirst(".//p[strong[contains(text(),'Aired')]]");
				if (airedP) {
                    QString airedText = airedP.text();
					airedText.remove("Aired:");
                    airedText = airedText.trimmed();
                    show.releaseDate = airedText;
				}
				auto seasonP = infoPanel.selectFirst(".//p[strong[contains(text(),'Season')]]");
                if (seasonP) extraLines << seasonP.text().trimmed();
				auto studioP = infoPanel.selectFirst(".//p[strong[contains(text(),'Studio')]]");
                if (studioP) extraLines << studioP.text().trimmed();
				// External links
				auto extP = infoPanel.selectFirst(".//p[contains(@class,'external-links')]");
				if (extP) {
					auto anchors = extP.select(".//a");
					if (!anchors.isEmpty()) {
						QString linkText = "External Links:<br>";
						for (int i = 0; i < anchors.size(); ++i) {
							const auto &a = anchors[i];
                            QString name = a.text().trimmed();
							QString href = a.attr("href");
							if (href.startsWith("//")) href = "https:" + href;
							
							if (name.isEmpty()) {
								linkText += QString("<a href=\"%1\">%1</a>").arg(href);
							} else {
								linkText += QString("<a href=\"%1\">%2</a>").arg(href, name);
							}
							
							if (i < anchors.size() - 1) linkText += "<br>";
						}
						extraLines << linkText;
					}
				}
			}
			if (!extraLines.isEmpty()) {
				if (!show.description.isEmpty()) show.description += "<br><br>";
                show.description += extraLines.join("<br>");
			}
		}
	}
	return totalCount;
}

QList<VideoServer> AnimePahe::loadServers(Client *client, const PlaylistItem *episode) const {
	Q_UNUSED(client);
	QList<VideoServer> servers;
	QString url = hostUrl();
    if (episode->link.startsWith("/")) url += episode->link.mid(1);
	else url += episode->link;
    auto doc = client->get(url, m_headers).toSoup();
	if (!doc) return servers;
	// buttons with qualities and kwik links
	auto buttons = doc.select("//div[@id='resolutionMenu']//button");
	Q_FOREACH (const auto &btn, buttons) {
		QString quality = btn.text().trimmed();
		QString kwik = btn.attr("data-src");
		if (kwik.isEmpty()) continue;
		if (kwik.startsWith("//")) kwik = "https:" + kwik;
		else if (kwik.startsWith("/")) kwik = "https://kwik.cx" + kwik; // fallback
		servers.emplaceBack("Kwik " + quality, kwik);
	}
	return servers;
}

PlayInfo AnimePahe::extractSource(Client *client, VideoServer &server) {
	PlayInfo info;
	QMap<QString, QString> getHeaders;
    getHeaders["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:144.0) Gecko/20100101 Firefox/144.0";
	getHeaders["Referer"] = "https://animepahe.ru/";
	Client::Response kwikResp = client->get(server.link, getHeaders);
    QString unpacked = Functions::jsUnpack(kwikResp.body);
    QString url;
    int idx = unpacked.indexOf("const source='");
    if (idx != -1) {
        int start = idx + strlen("const source='");
        int end = unpacked.indexOf("'", start);
        if (end != -1) {
            url = unpacked.mid(start, end - start);
        }
    }
    if (!url.isEmpty()) {
        info.videos.emplaceBack(QUrl(url), server.name);
        info.addHeader("Referer", "https://kwik.cx/");
        info.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:144.0) Gecko/20100101 Firefox/144.0");
    }
	
	return info;
}

