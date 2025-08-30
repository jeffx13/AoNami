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
    ~SearchManager() {
        if (m_watcher.isRunning()) {
            m_cancelled = true;
            try { m_watcher.waitForFinished(); } catch(...) {}
        }
    }
    void search(const QString& query,int page,int type, ShowProvider* provider);
    void latest(int page, int type, ShowProvider* provider);
    void popular(int page, int type, ShowProvider* provider);
    Q_INVOKABLE bool canFetchMore() { return !(isLoading() || m_watcher.isRunning() || !m_hasMore); }
    ShowData &getResultAt(int index) { return m_list[index]; }
    int count() const { return m_list.count(); }
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void fetchMore();
    Q_INVOKABLE void reload();

    Q_SIGNAL void aboutToInsert(int startIndex, int count);
    Q_SIGNAL void inserted();
    Q_SIGNAL void modelReset();
    Q_SIGNAL void countChanged(int count);
private:
    QFutureWatcher<QList<ShowData>> m_watcher;
    Client m_client{&m_cancelled};
    QList<ShowData> m_list;
    std::function<QList<ShowData>()> m_lastSearch;
    bool m_hasMore = false;
    int m_currentPage = 1;
};
