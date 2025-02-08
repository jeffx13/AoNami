#include "searchresultmanager.h"
#include "Providers/showprovider.h"
#include "utils/errorhandler.h"
#include "utils/logger.h"


SearchResultManager::SearchResultManager(QObject *parent) : QAbstractListModel(parent) {
    QObject::connect (&m_watcher, &QFutureWatcher<QList<ShowData>>::finished, this, [this](){
        if (!m_watcher.future().isValid()) {
            // Operation was cancelled
            oLog() << "Search" << "Operation cancelled: " << m_cancelReason;
            // ErrorHandler::instance().show ("Operation cancelled: " + m_cancelReason, "Error");
        } else if (!m_isCancelled) {
            try {
                auto results = m_watcher.result();
                m_canFetchMore = !results.isEmpty();
                if (m_currentPage > 1) {
                    const int oldCount = m_list.count();
                    beginInsertRows(QModelIndex(), oldCount, oldCount + results.count() - 1);
                    m_list.reserve(oldCount + results.count());
                    m_list += results;
                    endInsertRows();
                } else {
                    m_list.reserve(results.size());
                    m_list.swap(results);
                    emit layoutChanged();
                }
            }
            catch (const MyException& ex) {
                ex.show();
            } catch(const std::exception& ex) {
                ErrorHandler::instance().show (ex.what(), "Explorer Error");
            } catch(...) {
                ErrorHandler::instance().show ("Something went wrong", "Explorer Error");
            }
        }
        m_isCancelled = false;
        setIsLoading (false);
    });

}

void SearchResultManager::search(const QString &query, int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning()) return;
    setIsLoading (true);
    m_currentPage = page;
    m_watcher.setFuture(QtConcurrent::run(&ShowProvider::search, provider, &m_client, query, page, type));
}

void SearchResultManager::latest(int page, int type, ShowProvider* provider)
{
    if (m_watcher.isRunning()) return;
    setIsLoading (true);
    m_currentPage = page;
    m_watcher.setFuture(QtConcurrent::run(&ShowProvider::latest, provider, &m_client, page, type));
}

void SearchResultManager::popular(int page, int type, ShowProvider* provider){
    if (m_watcher.isRunning()) return;
    setIsLoading (true);
    m_currentPage = page;
    m_watcher.setFuture(QtConcurrent::run(&ShowProvider::popular, provider, &m_client, page, type));
}

void SearchResultManager::cancel() {
    if (m_watcher.isRunning()) {
        m_isCancelled = true;
    }
}


int SearchResultManager::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return m_list.count();
}

QVariant SearchResultManager::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    const ShowData& show = m_list.at(index.row());

    switch (role){
    case TitleRole:
        return show.title;
        break;
    case CoverRole:
        return show.coverUrl;
        break;
    default:
        break;
    }
    return QVariant();
}



float SearchResultManager::getContentY() const
{
    return m_contentY;
}

void SearchResultManager::setContentY(float newContentY)
{
    if (qFuzzyCompare(m_contentY, newContentY))
        return;
    m_contentY = newContentY;
    emit contentYChanged();
}
