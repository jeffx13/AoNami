#include "ui/searchmanager.h"
#include "providers/showprovider.h"
#include "ui/uibridge.h"
#include "app/logger.h"
#include <QtConcurrent/QtConcurrentRun>

SearchManager::SearchManager(QObject *parent)
    : ListModel(parent)
{
    connect(&m_watcher, &QFutureWatcher<QList<ShowData>>::finished,
            this, &SearchManager::onSearchFinished);
    connect(&m_watcher, &QFutureWatcher<QList<ShowData>>::started,
            this, &SearchManager::isLoadingChanged);
    connect(&m_watcher, &QFutureWatcher<QList<ShowData>>::finished,
            this, &SearchManager::isLoadingChanged);
}

int SearchManager::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_list.count();
}

QVariant SearchManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_list.size()) return {};
    const ShowData &show = m_list.at(index.row());
    switch (role) {
    case TitleRole:     return show.title;
    case CoverRole:     return show.coverUrl;
    case LinkRole:      return show.link;
    case LatestTxtRole: return show.latestTxt;
    default:            return {};
    }
}

QHash<int, QByteArray> SearchManager::roleNames() const {
    return {
        {TitleRole,     "title"},
        {CoverRole,     "cover"},
        {LinkRole,      "link"},
        {LatestTxtRole, "latestTxt"},
    };
}

void SearchManager::onSearchFinished() {
    bool wasCancelled = m_cancel.isCancelled();
    m_cancel.reset();

    if (!m_watcher.future().isValid() || wasCancelled)
        return;

    QList<ShowData> results;
    try {
        results = m_watcher.result();
    } catch (const AppException &ex) {
        ex.show();
        return;
    } catch (const std::exception &ex) {
        UiBridge::instance().showError(ex.what(), "Explorer Error");
        return;
    } catch (...) {
        UiBridge::instance().showError("Something went wrong", "Explorer Error");
        return;
    }

    m_hasMore = !results.isEmpty();
    // Page 1 with nothing found must still clear the previous results, or they look like a hit.
    if (!m_hasMore) {
        if (m_currentPage <= 1 && !m_list.isEmpty()) {
            beginResetModel();
            m_list.clear();
            endResetModel();
            emit countChanged(0);
        }
        return;
    }

    if (m_currentPage > 1) {
        int first = m_list.count();
        int last = first + results.count() - 1;
        beginInsertRows(QModelIndex(), first, last);
        m_list.reserve(first + results.count());
        m_list.append(std::move(results));
        endInsertRows();
    } else {
        beginResetModel();
        m_list = std::move(results);
        endResetModel();
    }
    emit countChanged(m_list.count());
}

void SearchManager::runSearch(int page, SearchFunc &&func) {
    if (m_watcher.isRunning()) return;
    m_currentPage = page;
    m_lastSearch = std::move(func);
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::search(const QString &query, int page, int type, ShowProvider *provider) {
    runSearch(page, [this, query, type, provider]() {
        Client client(m_cancel);
        return provider->search(&client, query, m_currentPage, type);
    });
}

void SearchManager::latest(int page, int type, ShowProvider *provider) {
    runSearch(page, [this, type, provider]() {
        Client client(m_cancel);
        return provider->latest(&client, m_currentPage, type);
    });
}

void SearchManager::popular(int page, int type, ShowProvider *provider) {
    runSearch(page, [this, type, provider]() {
        Client client(m_cancel);
        return provider->popular(&client, m_currentPage, type);
    });
}

void SearchManager::cancel() {
    if (m_watcher.isRunning()) {
        oLog() << "Search" << "Cancelling operation";
        m_cancel.cancel();
    }
}

void SearchManager::fetchMore() {
    if (!canFetchMore()) return;
    m_cancel.reset();
    ++m_currentPage;
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::reload() {
    if (m_watcher.isRunning()) return;
    m_cancel.reset();
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}
