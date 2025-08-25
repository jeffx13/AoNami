#pragma once
#include <QDebug>
#include "showprovider.h"
#include <QJsonArray>
#include "base/player/playinfo.h"

class HiAnime : public ShowProvider
{
public:
	explicit HiAnime(QObject *parent = nullptr) : ShowProvider(parent) { setPreferredServer(""); }
	QString name() const override { return "HiAnime"; }
	QString hostUrl() const override { return "https://hianime.to/"; }

	QList<QString>     getAvailableTypes() const override { return {"Anime"}; }
	QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
	QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
	QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
	QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
	PlayInfo           extractSource(Client *client, VideoServer &server) override;

private:
	int                loadShow  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
	QMap<QString, QString> m_headers = {
		{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:143.0) Gecko/20100101 Firefox/143.0"},
		{"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
		{"Referer", "https://hianime.to/"},
	};
	
};


