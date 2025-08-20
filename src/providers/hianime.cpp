#include "hianime.h"
#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonArray>

static inline QString ha_pct(const QString &s) { return QUrl::toPercentEncoding(s); }

QList<ShowData> HiAnime::search(Client *client, const QString &query, int page, int type) {
	Q_UNUSED(type);
	QList<ShowData> shows;
	if (query.trimmed().isEmpty()) return shows;
	QString url = m_apiBase + "search/" + ha_pct(query) + "/" + QString::number(page);
	QJsonObject root = client->get(url).toJsonObject();
	QJsonArray arr = root.value("searchYour").toArray();
	Q_FOREACH(const auto &v, arr) {
		QJsonObject o = v.toObject();
		QString title = o.value("name").toString();
		QString id = o.value("idanime").toString();
		QString cover = o.value("img").toString();
		if (title.isEmpty() || id.isEmpty()) continue;
		shows.emplaceBack(title, id, cover, this, "", ShowData::ANIME);
	}
	return shows;
}

QList<ShowData> HiAnime::popular(Client *client, int page, int typeIndex) {
	Q_UNUSED(typeIndex);
	QList<ShowData> shows;
	QString url = m_apiBase + "mix/popular/" + QString::number(page);
	QJsonObject root = client->get(url).toJsonObject();
	QJsonArray arr = root.value("mixAni").toArray();
	Q_FOREACH(const auto &v, arr) {
		QJsonObject o = v.toObject();
		QString title = o.value("name").toString();
		QString id = o.value("idanime").toString();
		QString cover = o.value("img").toString();
		if (title.isEmpty() || id.isEmpty()) continue;
		shows.emplaceBack(title, id, cover, this, "", ShowData::ANIME);
	}
	return shows;
}

QList<ShowData> HiAnime::latest(Client *client, int page, int typeIndex) {
	Q_UNUSED(typeIndex);
	// Use TV listing as a proxy for latest updates
	QList<ShowData> shows;
	QString url = m_apiBase + "mix/tv/" + QString::number(page);
	QJsonObject root = client->get(url).toJsonObject();
	QJsonArray arr = root.value("mixAni").toArray();
	Q_FOREACH(const auto &v, arr) {
		QJsonObject o = v.toObject();
		QString title = o.value("name").toString();
		QString id = o.value("idanime").toString();
		QString cover = o.value("img").toString();
		if (title.isEmpty() || id.isEmpty()) continue;
		shows.emplaceBack(title, id, cover, this, "", ShowData::ANIME);
	}
	return shows;
}

static int ha_parseEpisodes(ShowData &show, const QJsonObject &root, bool fillPlaylist) {
	QJsonArray arr = root.value("episodetown").toArray();
	int count = 0;
	Q_FOREACH(const auto &v, arr) {
		QJsonObject o = v.toObject();
		QString name = o.value("name").toString();
		QString epId = o.value("epId").toString();
		double order = o.value("order").toString().toDouble();
		if (fillPlaylist) show.addEpisode(0, (float)order, epId, name);
		count++;
	}
	return count;
}


int HiAnime::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const {
	int total = 0;
	// Episodes list
	{
		QString url = m_apiBase + "episode/" + show.link;
		QJsonObject root = client->get(url).toJsonObject();
		total = ha_parseEpisodes(show, root, getPlaylist);
	}
	if (getEpisodeCountOnly && !getPlaylist && !getInfo) return total;
	if (getInfo) {
		QString infoUrl = m_apiBase + "related/" + show.link;
		QJsonObject infoRoot = client->get(infoUrl).toJsonObject();
		QJsonArray infoArr = infoRoot.value("infoX").toArray();
		if (!infoArr.isEmpty()) {
			QJsonObject basic = infoArr.at(0).toObject();
			if (show.description.isEmpty()) show.description = basic.value("desc").toString();
			if (show.coverUrl.isEmpty()) show.coverUrl = basic.value("image").toString();
		}
		if (infoArr.size() > 1) {
			QJsonObject meta = infoArr.at(1).toObject();
			QJsonArray genres = meta.value("genre").toArray();
			Q_FOREACH(const auto &g, genres) show.genres.push_back(g.toString());
		}
	}
	return total;
}

QList<VideoServer> HiAnime::loadServers(Client *client, const PlaylistItem *episode) const {
	QList<VideoServer> servers;
	QString epId = episode->link;
	// epId may contain "?ep=NNN" already. API expects only the query part value
	int pos = epId.indexOf("ep=");
	QString epParam = pos >= 0 ? epId.mid(pos) : epId;
	if (epParam.startsWith("ep=")) epParam = epParam.mid(3);
	QString url = m_apiBase + "server/" + epParam;
	QJsonObject root = client->get(url).toJsonObject();
	auto add = [&](const char *key){
		QJsonArray arr = root.value(key).toArray();
		Q_FOREACH(const auto &v, arr) {
			QJsonObject o = v.toObject();
			QString server = o.value("server").toString();
			QString srcId = o.value("srcId").toString();
			if (srcId.isEmpty()) srcId = QString::number(o.value("srcId").toInt());
			if (server.isEmpty() || srcId.isEmpty()) continue;
			servers.emplaceBack(server, srcId);
		}
	};
	add("sub");
	add("dub");
	return servers;
}

PlayInfo HiAnime::extractSource(Client *client, VideoServer &server) {
	PlayInfo play;
	QString url = m_apiBase + "src-server/" + server.link;
	QJsonObject root = client->get(url).toJsonObject();
	QJsonArray arr = root.value("serverSrc").toArray();
	Q_FOREACH(const auto &v, arr) {
		QJsonObject o = v.toObject();
		QJsonArray rest = o.value("rest").toArray();
		Q_FOREACH(const auto &rv, rest) {
			QJsonObject ro = rv.toObject();
			QString file = ro.value("file").toString();
			QString type = ro.value("type").toString();
			if (file.isEmpty()) continue;
			play.videos.emplaceBack(QUrl(file));
			if (type.compare("hls", Qt::CaseInsensitive) == 0) {
				play.addHeader("origin", "https://hianime.to");
				play.addHeader("referer", "https://hianime.to/");
			}
		}
	}
	return play;
}


