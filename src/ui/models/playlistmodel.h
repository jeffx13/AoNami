#pragma once
#include "playlistmanager.h"
#include <QAbstractItemModel>

class PlaylistModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    PlaylistModel(PlaylistManager *playlistManager);
    Q_SIGNAL void selectionsChanged(const QModelIndex &index);
    Q_SIGNAL void scrollToCurrentIndex();

    Q_INVOKABLE QModelIndex getCurrentIndex(const QModelIndex &idx) const {
        auto currentPlaylist = static_cast<PlaylistItem*>(idx.internalPointer());
        if (!currentPlaylist ||
            !currentPlaylist->isValidIndex(currentPlaylist->getCurrentIndex()))
            return QModelIndex();
        return createIndex(currentPlaylist->getCurrentIndex(), 0, currentPlaylist->getCurrentItem().data());
    }

    enum
    {
        TitleRole = Qt::UserRole,
        IndexRole,
        NumberRole,
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
