#pragma once
#include <QAbstractListModel>
#include "base/librarymanager.h"


class LibraryModel: public QAbstractListModel
{
    Q_OBJECT
public:
    LibraryModel(LibraryManager *libraryManager): m_libraryManager(libraryManager) {
        connect(m_libraryManager, &LibraryManager::appended, this,
                [this](int startIndex, int count) {
                    beginInsertRows(QModelIndex(), startIndex, startIndex + count - 1);
                    endInsertRows();
                });

        connect(m_libraryManager, &LibraryManager::cleared, this,
                [this](int oldCount) {
                    beginRemoveRows(QModelIndex(), 0, oldCount - 1);
                    endRemoveRows();
                });

        connect(m_libraryManager, &LibraryManager::removed, this,
                [this](int index) {
                    beginRemoveRows(QModelIndex(), index, index);
                    endRemoveRows();
                });

        connect(m_libraryManager, &LibraryManager::moved, this,
                [this](int from, int to) {
                    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to > from ? to + 1 : to);
                    endMoveRows();
                });

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
        if (parent.isValid())
            return 0;
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
                auto lastWatchIndex = show["lastWatchedIndex"].toInt(-1);
                auto totalEpisodes = m_libraryManager->getTotalEpisodes(show["link"].toString());
                if (totalEpisodes > -1){
                    if (lastWatchIndex < 0) return totalEpisodes + 1;
                    return totalEpisodes - show["lastWatchedIndex"].toInt(0);
                }
                return 0;
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



