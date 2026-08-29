#pragma once
#include <QFutureWatcher>
#include <QVariantMap>
#include "net/client.h"
#include "providers/showdata.h"
#include "app/listmodel.h"
#include <qqmlintegration.h>

class SearchManager : public ListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(int  count     READ count     NOTIFY countChanged)
public:
    enum { TitleRole = Qt::UserRole, CoverRole, LinkRole, LatestTxtRole };

    explicit SearchManager(QObject *parent = nullptr);
    ~SearchManager() {
        if (m_watcher.isRunning()) {
            m_cancel.cancel();
            try { m_watcher.waitForFinished(); } catch (...) { qWarning("SearchManager: waitForFinished threw"); }
        }
    }

    void search(const QString &query, int page, int type, ShowProvider *provider);
    void latest(int page, int type, ShowProvider *provider);
    void popular(int page, int type, ShowProvider *provider);

    bool isLoading() const { return m_watcher.isRunning(); }

    // QML-facing manual pagination (Qt Quick views don't auto-call fetchMore).
    Q_INVOKABLE bool canFetchMore() const { return !m_watcher.isRunning() && m_hasMore; }
    Q_INVOKABLE void fetchMore();
    Q_INVOKABLE void reload();
    Q_INVOKABLE void cancel();
    // Force a view relayout (model reset) without changing data - e.g. on aspect-ratio change.
    Q_INVOKABLE void reset() { beginResetModel(); endResetModel(); }

    bool canFetchMore(const QModelIndex &parent) const override { Q_UNUSED(parent); return canFetchMore(); }
    void fetchMore(const QModelIndex &parent) override { Q_UNUSED(parent); fetchMore(); }

    ShowData &getResultAt(int index) {
        Q_ASSERT(index >= 0 && index < m_list.size());
        if (index < 0 || index >= m_list.size()) {
            static ShowData empty;
            return empty;
        }
        return m_list[index];
    }
    const ShowData &getResultAt(int index) const {
        Q_ASSERT(index >= 0 && index < m_list.size());
        if (index < 0 || index >= m_list.size()) {
            static const ShowData empty;
            return empty;
        }
        return m_list[index];
    }
    int count() const { return m_list.count(); }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_SIGNAL void countChanged(int count);
    Q_SIGNAL void isLoadingChanged();

private:
    using SearchFunc = std::function<QList<ShowData>()>;
    void runSearch(int page, SearchFunc &&func);
    void onSearchFinished();

    QFutureWatcher<QList<ShowData>> m_watcher;
    QList<ShowData> m_list;
    SearchFunc m_lastSearch;
    bool m_hasMore = false;
    int m_currentPage = 1;
};
