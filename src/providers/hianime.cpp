#include "hianime.h"
#include <QUrl>
#include <QRegularExpression>
#include <QSet>
#include <QPair>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

QList<ShowData> HiAnime::search(Client *client, const QString &query, int page, int type) {
	Q_UNUSED(type);
	QList<ShowData> shows;
	if (query.trimmed().isEmpty()) return shows;
	QString url = hostUrl() + "search?keyword=" + QUrl::toPercentEncoding(query) + "&page=" + QString::number(page);
	auto doc = client->get(url, m_headers).toSoup();
	if (!doc) return shows;

	auto items = doc.select("//div[contains(@class,'flw-item')]");
	for (int i = 0; i < items.size(); ++i) {
		auto item = items[i];
		auto a = item.selectFirst(".//div[contains(@class,'film-detail')]//a");
		if (!a) continue;
		QString href = a.attr("href");
		QString title = a.attr("title");
		if (title.isEmpty()) title = a.attr("data-jname");
		if (title.isEmpty()) title = a.text().trimmed();
		auto img = item.selectFirst(".//div[contains(@class,'film-poster')]//img");
		QString cover = img.attr("data-src");
		if (cover.isEmpty()) cover = img.attr("src");
		if (href.isEmpty() || title.isEmpty()) continue;
		int qpos = href.indexOf('?');
		if (qpos != -1) href = href.left(qpos);
		shows.emplaceBack(title, href, cover, this, "", ShowData::ANIME);
	}
	return shows;
}

QList<ShowData> HiAnime::popular(Client *client, int page, int typeIndex) {
	Q_UNUSED(typeIndex);
	QList<ShowData> shows;
	QString url = hostUrl() + "most-popular?page=" + QString::number(page);
	auto doc = client->get(url, m_headers).toSoup();
	if (!doc) return shows;

	auto items = doc.select("//div[contains(@class,'flw-item')]");
	for (int i = 0; i < items.size(); ++i) {
		auto item = items[i];
		auto a = item.selectFirst(".//div[contains(@class,'film-detail')]//a");
		if (!a) continue;
		QString href = a.attr("href");
		int qpos = href.indexOf('?');
		if (qpos != -1) href = href.left(qpos);
		QString title = a.attr("title");
		if (title.isEmpty()) title = a.attr("data-jname");
		if (title.isEmpty()) title = a.text().trimmed();
		auto img = item.selectFirst(".//div[contains(@class,'film-poster')]//img");
		QString cover = img.attr("data-src");
		if (cover.isEmpty()) cover = img.attr("src");
		if (href.isEmpty() || title.isEmpty()) continue;
		shows.emplaceBack(title, href, cover, this, "", ShowData::ANIME);
	}
	return shows;
}

QList<ShowData> HiAnime::latest(Client *client, int page, int typeIndex) {
	Q_UNUSED(typeIndex);
	QList<ShowData> shows;
	QString url = hostUrl() + "recently-updated?page=" + QString::number(page);
	auto doc = client->get(url, m_headers).toSoup();
	if (!doc) return shows;

	auto items = doc.select("//div[contains(@class,'flw-item')]");
	for (int i = 0; i < items.size(); ++i) {
		auto item = items[i];
		auto a = item.selectFirst(".//div[contains(@class,'film-detail')]//a");
		if (!a) continue;
		QString href = a.attr("href");
		int qpos = href.indexOf('?');
		if (qpos != -1) href = href.left(qpos);
		QString title = a.attr("title");
		if (title.isEmpty()) title = a.attr("data-jname");
		if (title.isEmpty()) title = a.text().trimmed();
		auto img = item.selectFirst(".//div[contains(@class,'film-poster')]//img");
		QString cover = img.attr("data-src");
		if (cover.isEmpty()) cover = img.attr("src");
		if (href.isEmpty() || title.isEmpty()) continue;
		shows.emplaceBack(title, href, cover, this, "", ShowData::ANIME);
	}
	return shows;
}

int HiAnime::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
	QString animePath = show.link;
	if (animePath.startsWith("/")) animePath = animePath.mid(1);
	QString animeUrl = hostUrl() + animePath;

	QString tmp = show.link;
	int qpos = tmp.indexOf('?');
	if (qpos != -1) tmp = tmp.left(qpos);
	if (tmp.endsWith("/")) tmp.chop(1);
	int dash = tmp.lastIndexOf('-');
	QString seriesId = dash != -1 ? tmp.mid(dash + 1) : QString();
	if (seriesId.isEmpty()) {
		static QRegularExpression re("(\\d+)$");
		auto m = re.match(tmp);
		if (m.hasMatch()) seriesId = m.captured(1);
	}

	int total = 0;
	if (!seriesId.isEmpty()) {
		QMap<QString, QString> ajaxHeaders = m_headers;
		ajaxHeaders["X-Requested-With"] = "XMLHttpRequest";
		ajaxHeaders["Accept"] = "*/*";
		ajaxHeaders["Referer"] = animeUrl;
		QString url = hostUrl() + "ajax/v2/episode/list/" + seriesId;
		auto root = client->get(url, ajaxHeaders).toJsonObject();

		QString html;
		if (root.contains("html")) html = root.value("html").toString();
		if (html.isEmpty() && root.contains("result")) {
			auto r = root.value("result").toObject();
			html = r.value("html").toString();
		}
		if (html.isEmpty()) {
			total = 0;
		} else {
			auto doc = CSoup::parse(html);
			auto nodes = doc.select("//a[contains(@class,'ep-item')]");
			if (nodes.isEmpty()) {
				total = 0;
			} else {
				struct Ep { double num; QString href; QString title; };
				QMap<double, QList<Ep>> epMap;
				for (int i = 0; i < nodes.size(); ++i) {
					auto n = nodes[i];
					bool ok = false;
					double num = n.attr("data-number").toDouble(&ok);
					QString href = n.attr("href");
					QString title = n.attr("title");
					if (title.isEmpty()) title = n.text().trimmed();
					if (!ok) {
						QString t = title;
						static QRegularExpression re("\\d+(?:\\.\\d+)?");
						auto m = re.match(t);
						if (m.hasMatch()) num = m.captured(0).toDouble(&ok);
					}
					if (!href.isEmpty()) epMap[num].append({num, href, title});
				}

				if (getPlaylist && !getEpisodeCountOnly) {
					for (auto it = epMap.begin(); it != epMap.end(); ++it) {
						const QList<Ep> eps = it.value();
						for (int j = 0; j < eps.size(); ++j) {
							const Ep &e = eps.at(j);
							show.addEpisode(0, (float)e.num, e.href, e.title);
						}
					}
				}
				total = nodes.size();
			}
		}

		if (getEpisodeCountOnly && !getPlaylist && !getInfo) return total;
	}

	if (getInfo) {
		auto doc = client->get(animeUrl, m_headers).toSoup();
		if (doc) {
			auto img = doc.selectFirst("//div[contains(@class,'anisc-poster')]//img");
			QString cover = img.attr("src");
			if (cover.isEmpty()) cover = img.attr("data-src");
			if (!cover.isEmpty()) show.coverUrl = cover;

			auto desc = doc.selectFirst("//div[contains(@class,'anisc-info')]//*[contains(@class,'description')] | //div[contains(@class,'film-desc')]");
			if (desc) show.description = desc.text().trimmed();

			auto statusNode = doc.selectFirst("//div[contains(@class,'anisc-info')]//*[contains(text(),'Status')]/following::*[1]");
			if (statusNode) show.status = statusNode.text().trimmed();

			auto genreNodes = doc.select("//div[contains(@class,'anisc-info')]//div[contains(@class,'item-list')]//a");
			if (!genreNodes.isEmpty()) {
				show.genres.clear();
				for (int i = 0; i < genreNodes.size(); ++i) show.genres.push_back(genreNodes[i].text().trimmed());
			}
		}
	}
	return total;
}

QList<VideoServer> HiAnime::loadServers(Client *client, const PlaylistItem *episode) const {
	QList<VideoServer> servers;
	QString epPath = episode->link;
	int pos = epPath.indexOf("?ep=");
	QString episodeId;
	if (pos != -1) episodeId = epPath.mid(pos + 4);
	if (episodeId.isEmpty()) return servers;

	QString referer = hostUrl() + (epPath.startsWith("/") ? epPath.mid(1) : epPath);
	QMap<QString, QString> ajaxHeaders = m_headers;
	ajaxHeaders["X-Requested-With"] = "XMLHttpRequest";
	ajaxHeaders["Accept"] = "*/*";
	ajaxHeaders["Referer"] = referer;

	QString listUrl = hostUrl() + "ajax/v2/episode/servers?episodeId=" + episodeId;
	auto root = client->get(listUrl, ajaxHeaders).toJsonObject();
	QString html;
	if (root.contains("html")) html = root.value("html").toString();
	if (html.isEmpty() && root.contains("result")) html = root.value("result").toObject().value("html").toString();
	if (html.isEmpty()) return servers;

	auto doc = CSoup::parse(html);
	QVector<QString> groups; groups << "servers-sub" << "servers-dub" << "servers-mixed" << "servers-raw";
	QSet<QString> allowedHosts; allowedHosts << "HD-1" << "HD-2" << "HD-3" << "StreamTape";
	for (int g = 0; g < groups.size(); ++g) {
		QString grp = groups[g];
		auto items = doc.select(QString("//div[contains(@class,'%1')]//div[contains(@class,'item')]").arg(grp));
		QString typeLabel;
		if (grp.contains("sub")) typeLabel = "Sub";
		else if (grp.contains("dub")) typeLabel = "Dub";
		else if (grp.contains("mixed")) typeLabel = "Mixed";
		else if (grp.contains("raw")) typeLabel = "Raw";
		for (int i = 0; i < items.size(); ++i) {
			auto it = items[i];
			QString id = it.attr("data-id");
			QString hostName = it.text().trimmed();
			if (id.isEmpty() || hostName.isEmpty()) continue;
			if (!allowedHosts.contains(hostName)) continue;
			QString name = hostName + " - " + typeLabel;
            servers.emplaceBack(name, id);
		}
	}

	QString pref = getPreferredServer();
	if (!pref.isEmpty()) {
		QList<VideoServer> preferred; QList<VideoServer> others;
		for (int i = 0; i < servers.size(); ++i) {
			const auto &s = servers[i];
			if (s.name.contains(pref, Qt::CaseInsensitive)) preferred.append(s); else others.append(s);
		}
		servers.clear();
		servers += preferred; servers += others;
	}
	return servers;
}

PlayInfo HiAnime::extractSource(Client *client, VideoServer &server) {
	PlayInfo info;	

	QMap<QString, QString> headers = m_headers;
	headers["X-Requested-With"] = "XMLHttpRequest";
	headers["Accept"] = "*/*";
	headers["Referer"] = hostUrl();

	QString srcUrl = hostUrl() + "ajax/v2/episode/sources?id=" + server.link;
	auto srcRoot = client->get(srcUrl, headers).toJsonObject();
	QString resolvedLink = srcRoot.value("link").toString();
	if (resolvedLink.isEmpty() && srcRoot.contains("data")) resolvedLink = srcRoot.value("data").toObject().value("link").toString();
	if (resolvedLink.isEmpty()) return info;

	// MegaCloud: HD-1 / HD-2 / HD-3
	if (server.name.startsWith("HD-")) {
		QString googleApi = "https://script.google.com/macros/s/AKfycbxHbYHbrGMXYD2-bC-C43D3njIbU-wGiYQuJL61H4vyy6YVXkybMNNEPJNPPuZrD1gRVA/exec";
		QUrl url(resolvedLink);
		QString host = url.host();
		if (host.isEmpty()) return info;
		QString base = "https://" + host;
		headers["Referer"] = base + "/";

		QString page = client->get(resolvedLink, headers).body;
		QString nonce;
		{
            static QRegularExpression re1("\\b[a-zA-Z0-9]{48}\\b");
			auto m1 = re1.match(page);
			if (m1.hasMatch()) nonce = m1.captured(0);
			if (nonce.isEmpty()) {
                static QRegularExpression re2("\\b([a-zA-Z0-9]{16})\\b.*?\\b([a-zA-Z0-9]{16})\\b.*?\\b([a-zA-Z0-9]{16})\\b");
				auto m2 = re2.match(page);
				if (m2.hasMatch()) nonce = m2.captured(1) + m2.captured(2) + m2.captured(3);
			}
		}
		if (nonce.isEmpty()) return info;

		QString id;
		{
			QString link = resolvedLink;
			int idx = link.indexOf("/e-1/");
			if (idx != -1) {
				int start = idx + 5;
				int end = link.indexOf('?', start);
				id = end == -1 ? link.mid(start) : link.mid(start, end - start);
			} else {
				QUrlQuery q(url);
				id = q.queryItemValue("id");
			}
		}
		if (id.isEmpty()) return info;

		QString sourcesUrl = base + "/embed-2/v3/e-1/getSources?id=" + id + "&_k=" + nonce;
		QJsonObject src = client->get(sourcesUrl, headers).toJsonObject();
		if (src.isEmpty()) return info;

		if (src.contains("tracks")) {
			QJsonArray tracks = src.value("tracks").toArray();
			for (int i = 0; i < tracks.size(); ++i) {
				QJsonObject t = tracks.at(i).toObject();
				QString kind = t.value("kind").toString();
				if (kind != "captions") continue;
				QString subUrl = t.value("file").toString();
				QString label = t.value("label").toString();
				if (!subUrl.isEmpty()) info.subtitles.emplaceBack(QUrl(subUrl), label);
			}
		}

		bool encrypted = src.value("encrypted").toBool(true);
		QJsonArray sources = src.value("sources").toArray();
		for (int i = 0; i < sources.size(); ++i) {
			QJsonObject s = sources.at(i).toObject();
			QString file = s.value("file").toString();
			QString m3u8 = file;
			if (encrypted && !file.contains(".m3u8")) {
				QJsonObject keysJson = client->get("https://raw.githubusercontent.com/yogesh-hacker/MegacloudKeys/refs/heads/main/keys.json").toJsonObject();
				QString key = keysJson.value("mega").toString();
				if (key.isEmpty()) continue;
				QString fullUrl = googleApi
					+ "?encrypted_data=" + QUrl::toPercentEncoding(file)
					+ "&nonce=" + QUrl::toPercentEncoding(nonce)
					+ "&secret=" + QUrl::toPercentEncoding(key);
				QString dec = client->get(fullUrl).body;
                static QRegularExpression reFile("\"file\":\"(.*?)\"");
				auto m = reFile.match(dec);
				if (m.hasMatch()) m3u8 = m.captured(1);
			}
			if (!m3u8.isEmpty()) {
				info.videos.emplaceBack(QUrl(m3u8), "");
				info.addHeader("Referer", base + "/");
			}
		}
		return info;
	}

	// StreamTape
	if (server.name.startsWith("StreamTape")) {
		QString baseUrl = "https://streamtape.com/e/";
		QString newUrl;
		if (resolvedLink.startsWith(baseUrl)) {
			newUrl = resolvedLink;
		} else {
			QStringList parts = resolvedLink.split('/');
			if (parts.size() > 4) {
				newUrl = baseUrl + parts.at(4);
			}
		}
		if (newUrl.isEmpty()) return info;

		QString html = client->get(newUrl).body;
		int pos = html.indexOf("document.getElementById('robotlink')");
		if (pos == -1) return info;
		QString script = html.mid(pos);
		QString urlPart1;
		int start = script.indexOf("innerHTML = '");
		if (start != -1) {
			start += 14; // length of "innerHTML = '"
			int end = script.indexOf('\'', start);
			if (end != -1) urlPart1 = script.mid(start, end - start);
		}
		if (urlPart1.isEmpty()) return info;
		int plusPos = script.indexOf("+ ('xcd", start);
		QString urlPart2;
		if (plusPos != -1) {
			plusPos += 7; // skip "+ ('xcd"
			int end2 = script.indexOf('\'', plusPos);
			if (end2 != -1) urlPart2 = script.mid(plusPos, end2 - plusPos);
		}
		QString finalUrl = "https:" + urlPart1 + urlPart2;
		if (!finalUrl.isEmpty()) {
			info.videos.emplaceBack(QUrl(finalUrl), "Streamtape");
		}
		return info;
	}

	return info;
}
