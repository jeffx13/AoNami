#pragma once
#include "playlistmanager.h"
#include <QAbstractListModel>

class PlaylistModel: public QAbstractItemModel
{
    Q_OBJECT
public:
    PlaylistModel(PlaylistManager *playlistManager);
    Q_SIGNAL void selectionsChanged(QModelIndex index, bool);


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
