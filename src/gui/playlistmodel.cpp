#include "playlistmodel.h"
#include "playlistitem.h"

int PlaylistModel::rowCount(const QModelIndex &parent) const {
    if (parent.column() > 0) return 0;
    if (!parent.isValid()) {
        // top level: number of playlists in root
        return m_playlistManager->count(); // or m_playlistManager->root()->size()
    }
    auto parentItem = static_cast<PlaylistItem*>(parent.internalPointer());
    return parentItem ? parentItem->size() : 0;
}

QModelIndex PlaylistModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    PlaylistItem *parentItem = parent.isValid()
                                   ? static_cast<PlaylistItem*>(parent.internalPointer())
                                   : m_playlistManager->root();

    if (!parentItem) return QModelIndex();

    PlaylistItem *childItem = parentItem->at(row);
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex PlaylistModel::parent(const QModelIndex &childIndex) const {
    if (!childIndex.isValid()) return QModelIndex();

    PlaylistItem *childItem = static_cast<PlaylistItem*>(childIndex.internalPointer());
    PlaylistItem *parentItem = childItem ? childItem->parent() : nullptr;
    if (!parentItem || parentItem == m_playlistManager->root())
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {
    // if (!index.isValid() || m_root->isEmpty())
        // return QVariant();
    if (!index.isValid())
        return QVariant();

    auto item = static_cast<PlaylistItem*>(index.internalPointer());

    switch (role) {
    case TitleRole:
        return item->name;
    case IndexRole:
        return index;
    case NumberRole:
        return item->number;
    case NumberTitleRole:
        return item->type == PlaylistItem::LIST ? item->name : item->getFullName();
    case IsCurrentIndexRole:
        if (!item->parent() || item->parent()->getCurrentIndex() == -1) return false;
        return item->parent()->getCurrentItem() == item;
    case IsDeletableRole:
        return item->size() > 0 || item->type == PlaylistItem::PASTED;
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
    QHash<int, QByteArray> names
        {
            {TitleRole, "title"},
            {NumberRole, "number"},
            {IndexRole, "index"},
            {NumberTitleRole, "numberTitle"},
            {IsCurrentIndexRole, "isCurrentIndex"},
            {IsDeletableRole, "isDeletable"}
        };
    return names;
}

