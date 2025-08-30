#include "searchresultmodel.h"
#include "base/searchmanager.h"
#include "base/showdata.h"

SearchResultModel::SearchResultModel(SearchManager *searchManager, QObject *parent)
    : m_searchManager(searchManager), QAbstractListModel(parent) {
    connect(m_searchManager, &SearchManager::aboutToInsert, this,
            [this](int startIndex, int endIndex) {
        beginInsertRows(QModelIndex(), startIndex, endIndex);
    });
    connect(m_searchManager, &SearchManager::inserted, this, &SearchResultModel::endInsertRows);
    connect(m_searchManager, &SearchManager::modelReset, this, &SearchResultModel::reset);
}

void SearchResultModel::reset() {
    beginResetModel();
    endResetModel();
}

int SearchResultModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_searchManager->count();
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    const ShowData& show = m_searchManager->getResultAt(index.row());

    switch (role){
    case TitleRole:
        return show.title;
    case CoverRole:
        return show.coverUrl;
    case LinkRole:
        return show.link;
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> SearchResultModel::roleNames() const {
    QHash<int, QByteArray> names;
    names[TitleRole] = "title";
    names[CoverRole] = "cover";
    names[LinkRole] = "link";
    return names;
}


