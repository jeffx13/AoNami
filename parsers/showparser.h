#ifndef SHOWPARSER_H
#define SHOWPARSER_H

#include "episode.h"
#include "showresponse.h"

#include <QFutureWatcher>
#include <QNetworkReply>
#include <QString>

#include <debug/debugger.h>

#include <network/client.h>



class ShowParser: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT);
    Q_PROPERTY(int providerEnum READ providerEnum CONSTANT);
protected:
    bool m_canFetchMore;
    int m_currentPage;
    NetworkClient client;
    QFutureWatcher<void> watcher;
    QFutureWatcher<QVector<ShowResponse>> searchWatcher;
    virtual int providerEnum() = 0;
public:
    ShowParser();
    virtual QString name() = 0;
    virtual std::string hostUrl() = 0;
    virtual QVector<ShowResponse> search(QString query,int page,int type) = 0;
    virtual QVector<ShowResponse> popular(int page,int type) = 0;
    virtual QVector<ShowResponse> latest(int page,int type) = 0;
    virtual bool canFetchMore() final {return m_canFetchMore;};
    virtual QVector<ShowResponse> fetchMore() = 0;
    virtual void loadDetail(ShowResponse* show)  = 0;
    virtual QVector<VideoServer> loadServers(Episode* episode) = 0;
    virtual void extractSource(VideoServer* server) = 0;

signals:
    void episodesFetched(QVector<Episode> results);
    void sourceFetched(QString link);
    void loadingEnd();
};

#endif // SHOWPARSER_H
