#pragma once
#include "playlistmanager.h"
#include <QAbstractListModel>

class PlaylistModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    PlaylistModel(PlaylistManager *playlistManager) : m_playlistManager(playlistManager) {
        connect(m_playlistManager, &PlaylistManager::inserted, this,
                [this](PlaylistItem *parent, int index) {
                    auto parentIndex = parent->parent() ? createIndex(parent->parent()->getCurrentIndex(), 0, parent->getCurrentItem()) : QModelIndex();
                    beginInsertRows(parentIndex, index, index);
                    endInsertRows();
                    emit layoutChanged();
                });

        connect(m_playlistManager, &PlaylistManager::removed, this,
                [this](QModelIndex modelIndex) {
                    beginRemoveRows(modelIndex.parent(), modelIndex.row(), modelIndex.row());
                    endRemoveRows();
                });

        connect(m_playlistManager, &PlaylistManager::modelReset, this,
                [this]() {
                    beginResetModel();
                    endResetModel();
                });

        connect(m_playlistManager, &PlaylistManager::updateSelections, this,
                [this](int playlistIndex, int itemIndex, bool scrollToIndex) {
                    if (playlistIndex == -1 || itemIndex == -1) {
                        emit selectionsChanged(QModelIndex(), false);
                        return;
                    }
                    auto parentIndex = index(playlistIndex, 0, QModelIndex());
                    auto childIndex = index(itemIndex, 0, parentIndex);
                    emit selectionsChanged(childIndex, scrollToIndex);

                });

        connect(m_playlistManager, &PlaylistManager::changed, this,
                [this](int i) {
                    emit dataChanged(index(i, 0, QModelIndex()), index(i, 0, QModelIndex()));
                });
    }
    Q_SIGNAL void selectionsChanged(QModelIndex, bool);


    Q_INVOKABLE QModelIndex getCurrentIndex(QModelIndex i) const {
        auto currentPlaylist = static_cast<PlaylistItem *>(i.internalPointer());
        if (!currentPlaylist ||
            !currentPlaylist->isValidIndex(currentPlaylist->getCurrentIndex()))
            return QModelIndex();
        return index(currentPlaylist->getCurrentIndex(), 0, index(currentPlaylist->parent()->indexOf(currentPlaylist), 0, QModelIndex()));
    }

    // QAbstractItemModel interface
    enum
    {
        TitleRole = Qt::UserRole,
        IndexRole,
        NumberRole,
        NumberTitleRole,
        IsCurrentIndexRole,
        IsDeletableRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &childIndex) const override;
    int columnCount(const QModelIndex &parent) const override { return 1; }

private:
    PlaylistManager *m_playlistManager;
};
