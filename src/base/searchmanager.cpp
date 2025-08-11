#include "searchmanager.h"
#include "Providers/showprovider.h"
#include "gui/errordisplayer.h"
#include "app/logger.h"
#include <QtConcurrent/QtConcurrentRun>

SearchManager::SearchManager(QObject *parent) : ServiceManager(parent) {
    QObject::connect (&m_watcher, &QFutureWatcher<QList<ShowData>>::finished, this, [this](){
        if (!m_watcher.future().isValid()) {
            // Operation was cancelled
            oLog() << "Search" << "Operation cancelled: " << m_cancelReason;
            // ErrorDisplayer::instance().show ("Operation cancelled: " + m_cancelReason, "Error");
        } else if (!m_isCancelled) {
            try {
                auto results = m_watcher.result();
                m_canFetchMore = !results.isEmpty();
                if (m_currentPage > 1) {
                    const int oldCount = m_list.count();
                    // m_list.reserve(oldCount + results.count());
                    m_list += results;
                    emit appended(oldCount, results.count());

                } else {
                    if (!m_list.isEmpty()) {
                        int count = m_list.count();
                        m_list.clear();
                        emit cleared(count);
                    }
                    m_list = results;
                    emit appended(0, results.count());
                    // emit layoutChanged();
                }
            }
            catch (const MyException& ex) {
                ex.show();
            } catch(const std::exception& ex) {
                ErrorDisplayer::instance().show (ex.what(), "Explorer Error");
            } catch(...) {
                ErrorDisplayer::instance().show ("Something went wrong", "Explorer Error");
            }
        }
        m_isCancelled = false;
        setIsLoading(false);
    });

}

void SearchManager::search(const QString &query, int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning()) return;
    setIsLoading (true);
    m_currentPage = page;
    m_watcher.setFuture(QtConcurrent::run(&ShowProvider::search, provider, &m_client, query, page, type));
}

void SearchManager::latest(int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning()) return;
    setIsLoading (true);
    m_currentPage = page;
    m_watcher.setFuture(QtConcurrent::run(&ShowProvider::latest, provider, &m_client, page, type));
}

void SearchManager::popular(int page, int type, ShowProvider* provider){
    if (m_watcher.isRunning()) return;
    setIsLoading (true);
    m_currentPage = page;
    m_watcher.setFuture(QtConcurrent::run(&ShowProvider::popular, provider, &m_client, page, type));
}

void SearchManager::cancel() {
    if (m_watcher.isRunning()) {
        oLog() << "Search" << "Cancelling operation";
        m_isCancelled = true;
    }
}


// int SearchManager::rowCount(const QModelIndex &parent) const {
//     if (parent.isValid())
//         return 0;
//     return m_list.count();
// }

// QVariant SearchManager::data(const QModelIndex &index, int role) const {
//     if (!index.isValid())
//         return QVariant();

//     const ShowData& show = m_list.at(index.row());

//     switch (role){
//     case TitleRole:
//         return show.title;
//         break;
//     case CoverRole:
//         return show.coverUrl;
//         break;
//     default:
//         break;
//     }
//     return QVariant();
// }



float SearchManager::getContentY() const
{
    return m_contentY;
}

void SearchManager::setContentY(float newContentY)
{
    if (qFuzzyCompare(m_contentY, newContentY))
        return;
    m_contentY = newContentY;
    emit contentYChanged();
}
