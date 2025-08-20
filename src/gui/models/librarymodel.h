#pragma once
#include <QAbstractListModel>
#include "base/librarymanager.h"
#include "logger.h"


class LibraryModel: public QAbstractListModel
{
    Q_OBJECT
public:
    LibraryModel(LibraryManager *libraryManager): m_libraryManager(libraryManager) {
        connect(m_libraryManager, &LibraryManager::aboutToInsert, this,
                [this](int startIndex, int count) {
                    beginInsertRows(QModelIndex(), startIndex, startIndex + count - 1);
                });
        connect(libraryManager, &LibraryManager::inserted, this,  &LibraryModel::endInsertRows);

        connect(m_libraryManager, &LibraryManager::modelReset, this,
                [this]() {
                    beginResetModel();
                    endResetModel();
                });

        connect(m_libraryManager, &LibraryManager::aboutToRemove, this,
                [this](int index) {
                    beginRemoveRows(QModelIndex(), index, index);
                });

        connect(libraryManager, &LibraryManager::removed, this,  &LibraryModel::endRemoveRows);

        connect(m_libraryManager, &LibraryManager::aboutToMove, this,
                [this](int from, int to) {
                    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to + (to > from ? 1 : 0));
                });

        connect(libraryManager, &LibraryManager::moved, this,  &LibraryModel::endMoveRows);

        connect(m_libraryManager, &LibraryManager::changed, this,
                [this](int showIndex) {
                    emit dataChanged(index(showIndex), index(showIndex));
                });
    }
    enum {
        TitleRole = Qt::UserRole,
        CoverRole,
        UnwatchedEpisodesRole,
        TypeRole,
    };

    int rowCount(const QModelIndex &parent) const {
        if (parent.isValid()) return 0;
        return m_libraryManager->count();
    }

    QVariant data(const QModelIndex &index, int role) const {
        if (!index.isValid())
            return QVariant();

        switch (role){
        case TitleRole:
            return m_libraryManager->getData(index.row(), "title");
        case CoverRole:
            return m_libraryManager->getData(index.row(), "cover");
        case UnwatchedEpisodesRole:
        {
            auto episodeData = m_libraryManager->getData(index.row(), "last_watched_index,total_episodes").toMap();
            auto totalEpisodes = episodeData.value("total_episodes", 0).toLongLong();
            auto lastWatchedIndex = episodeData.value("last_watched_index", -1).toLongLong();
            if (totalEpisodes < 0) return 0;
            if (lastWatchedIndex < 0) return totalEpisodes;
            return totalEpisodes - lastWatchedIndex - 1;
        }
        case TypeRole:
            return m_libraryManager->getData(index.row(), "type");
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const {
        QHash<int, QByteArray> names;
        names[TitleRole] = "title";
        names[CoverRole] = "cover";
        names[UnwatchedEpisodesRole] = "unwatchedEpisodes";
        return names;
    }
private:
    LibraryManager *m_libraryManager;

};



