#include "searchmanager.h"
#include "providers/showprovider.h"
#include "ui/uibridge.h"
#include "app/logger.h"
#include <QtConcurrent/QtConcurrentRun>

SearchManager::SearchManager(QObject *parent)
    : ServiceManager(parent)
{
    connect(&m_watcher, &QFutureWatcher<QList<ShowData>>::finished, this, [this]() {
        setIsLoading(false);
        m_cancelled = false;
        if (!m_watcher.future().isValid() || m_cancelled)
            return;
        QList<ShowData> results;
        try { results = m_watcher.result(); } 
        catch (const AppException& ex) { ex.show(); } 
        catch (const std::exception& ex) { UiBridge::instance().showError(ex.what(), "Explorer Error"); } 
        catch (...) {UiBridge::instance().showError("Something went wrong", "Explorer Error"); }
        m_hasMore = !results.isEmpty();
        if (!m_hasMore) return;
        if (m_currentPage > 1) {
            emit aboutToInsert(m_list.count(), m_list.count() + results.count() - 1);
            m_list.reserve(m_list.count() + results.count());
            m_list += results;
            emit inserted();
        } else {
            m_list.swap(results);
            emit modelReset();
        }
        emit countChanged(m_list.count());
    });
}

void SearchManager::search(const QString &query, int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning())
        return;
    setIsLoading(true);
    m_currentPage = page;
    m_lastSearch = [this, query, type, provider]() {
        return provider->search(&m_client, query, m_currentPage, type);
    };
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::latest(int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning())
        return;
    setIsLoading(true);
    m_currentPage = page;
    m_lastSearch = [this, type, provider]() {
        return provider->latest(&m_client, m_currentPage, type);
    };
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::popular(int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning())
        return;
    setIsLoading(true);
    m_currentPage = page;
    m_lastSearch = [this, type, provider]() {
        return provider->popular(&m_client, m_currentPage, type);
    };
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::cancel()
{
    if (m_watcher.isRunning()) {
        oLog() << "Search" << "Cancelling operation";
        m_cancelled = true;
    }
}

void SearchManager::fetchMore()
{
    if (!canFetchMore()) return;
    setIsLoading(true);
    ++m_currentPage;
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::reload()
{
    if (m_watcher.isRunning())
        return;
    setIsLoading(true);
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}
