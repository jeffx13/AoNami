#pragma once
#include <QAbstractListModel>
#include "base/librarymanager.h"


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
        try{
            auto show = m_libraryManager->getShowJsonAt(index.row());
            switch (role){
            case TitleRole:
                return show["title"].toString();
            case CoverRole:
                return show["cover"].toString();
            case UnwatchedEpisodesRole:
            {
                // auto lastWatchIndex = show["lastWatchedIndex"].toInt(-1);
                auto totalEpisodes = m_libraryManager->getTotalEpisodes(show["link"].toString());
                // return totalEpisodes;
                if (totalEpisodes < 0) return 0;
                return totalEpisodes - show["lastWatchedIndex"].toInt(0) - 1;
            }
            case TypeRole:
                return show["type"].toInt(0);
            }
        }catch(...)
        {
            return {};
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



