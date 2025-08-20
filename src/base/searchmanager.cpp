#include "searchmanager.h"
#include "providers/showprovider.h"
#include "gui/errordisplayer.h"
#include "app/logger.h"
#include <QtConcurrent/QtConcurrentRun>

SearchManager::SearchManager(QObject *parent) : ServiceManager(parent) {
    QObject::connect(&m_watcher, &QFutureWatcher<QList<ShowData>>::finished, this, [this]() {
        if (m_watcher.future().isValid()&& !m_isCancelled) {
            try {
                auto results = m_watcher.result();
                m_canFetchMore = !results.isEmpty();
                if (m_currentPage > 1) {
                    const int oldCount = m_list.count();
                    m_list += results;
                    emit appended(oldCount, results.count());
                } else {
                    if (!m_list.isEmpty()) {
                        int count = m_list.count();
                        m_list.clear();
                        emit modelReset();
                    }
                    m_list = results;
                    emit appended(0, results.count());
                }
            }
            catch (const MyException& ex) {
                ex.show();
            } catch (const std::exception& ex) {
                ErrorDisplayer::instance().show(ex.what(), "Explorer Error");
            } catch (...) {
                ErrorDisplayer::instance().show("Something went wrong", "Explorer Error");
            }
        } else {

        }
        m_isCancelled = false;
        setIsLoading(false);
    });
}

void SearchManager::search(const QString &query, int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning()) return;
    setIsLoading(true);
    m_currentPage = page;
    m_lastSearch = [this, query, type, provider]() {
        return provider->search(&m_client, query, m_currentPage, type);
    };
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::latest(int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning()) return;
    setIsLoading(true);
    m_currentPage = page;
    m_lastSearch = [this, type, provider]() {
        return provider->latest(&m_client, m_currentPage, type);
    };
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::popular(int page, int type, ShowProvider* provider){
    if (m_watcher.isRunning()) return;
    setIsLoading(true);
    m_currentPage = page;
    m_lastSearch = [this, type, provider]() {
        return provider->popular(&m_client, m_currentPage, type);
    };
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::cancel() {
    if (m_watcher.isRunning()) {
        oLog() << "Search" << "Cancelling operation";
        m_isCancelled = true;
    }
}

void SearchManager::fetchMore() {
    if (m_watcher.isRunning() || !m_canFetchMore) return;
    setIsLoading(true);
    ++m_currentPage;
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}

void SearchManager::reload() {
    if (m_watcher.isRunning()) return;
    setIsLoading(true);
    m_watcher.setFuture(QtConcurrent::run(m_lastSearch));
}
