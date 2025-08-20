#pragma once
#include <QAbstractListModel>
#include <QFutureWatcher>
#include "base/network/network.h"
#include "showdata.h"
#include "base/servicemanager.h"


class SearchManager: public ServiceManager
{
    Q_OBJECT
public:
    explicit SearchManager(QObject *parent = nullptr);

    void search(const QString& query,int page,int type, ShowProvider* provider);
    void latest(int page, int type, ShowProvider* provider);
    void popular(int page, int type, ShowProvider* provider);
    bool canLoadMore() { return !(m_isLoading || m_watcher.isRunning() || !m_canFetchMore); }
    ShowData &getResultAt(int index) { return m_list[index]; }
    int count() const { return m_list.count(); }
    Q_INVOKABLE void cancel();

    Q_SIGNAL void appended(int, int);
    Q_SIGNAL void cleared(int);
    Q_SIGNAL void modelReset();
private:
    QFutureWatcher<QList<ShowData>> m_watcher;
    std::atomic<bool> m_isCancelled = false;
    Client m_client{&m_isCancelled};
    QList<ShowData> m_list;
    QString m_cancelReason;

    int m_currentPage;
    bool m_canFetchMore = false;
};
