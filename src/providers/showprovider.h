#pragma once
#include "app/exception.h"
#include "net/client.h"
#include "net/html.h"
#include "providers/showdata.h"
#include "media/playlistitem.h"
#include "media/playinfo.h"
#include "media/danmaku.h"
#include "providers/jsunpack.h"
#include <QMutex>

class ShowProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT);
    Q_PROPERTY(QString hostUrl READ hostUrl CONSTANT);
    Q_PROPERTY(bool supportsDanmaku READ supportsDanmaku CONSTANT);
public:
    ShowProvider(QObject *parent = nullptr) : QObject(parent) {};
    virtual QString name() const = 0;
    virtual QString hostUrl() const = 0;
    virtual QList<QString> getAvailableTypes() const = 0;

    void setPreferredServer(const QString &serverName) {
        QMutexLocker lock(&m_prefServerMutex);
        m_preferredServer = serverName;
    }
    QString getPreferredServer() const {
        QMutexLocker lock(&m_prefServerMutex);
        return m_preferredServer;
    }

    [[nodiscard]] virtual QList<ShowData>    search         (Client *client, const QString &query, int page, int type) = 0;
    [[nodiscard]] virtual QList<ShowData>    popular        (Client *client, int page, int typeIndex) = 0;
    [[nodiscard]] virtual QList<ShowData>    latest         (Client *client, int page, int typeIndex) = 0;
                          int                loadShow       (Client *client, ShowData &show) const { return loadShow(client, show, false, show.getPlaylist() == nullptr, true); }
                          int                getEpisodeCount(Client *client, ShowData &show) const { return loadShow(client, show, true,  false, false); }
                          void               getPlaylist    (Client *client, ShowData &show) const { loadShow(client, show, false, true,  false); }
    [[nodiscard]] virtual QList<VideoServer> loadServers    (Client *client, const PlaylistItem *episode) const = 0;

    // VideoServer by value: each concurrent caller gets its own copy to mutate freely.
    [[nodiscard]] virtual PlayInfo           extractSource  (Client *client, VideoServer server) = 0;

    virtual bool supportsDanmaku() const { return false; }

protected:
    virtual int loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const = 0;

    float resolveTitleNumber(QString &title) const;

    // Providers call this from extractSource once they know the episode's key.
    bool attachDanmaku(PlayInfo &info, QList<DanmakuComment> comments,
                       const QString &cacheKey) const;

    QString m_preferredServer;
    mutable QMutex m_prefServerMutex;
};