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

	QList<QString> getAvailableTypes() const override { return {"Anime"}; }
	QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
	QList<ShowData>    popular      (Client *client, int page, int typeIndex) override;
	QList<ShowData>    latest       (Client *client, int page, int typeIndex) override;
	QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
	PlayInfo           extractSource(Client *client, VideoServer &server) override;

private:
	int                loadShow  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo = true) const override;
	const QString m_apiBase = "https://aniwatch-api-v1-0.onrender.com/api/";
};


