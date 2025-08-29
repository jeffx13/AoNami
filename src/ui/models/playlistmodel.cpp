#include "playlistmodel.h"
#include "playlistitem.h"

PlaylistModel::PlaylistModel(PlaylistManager *playlistManager) : m_playlistManager(playlistManager) {
    connect(m_playlistManager, &PlaylistManager::aboutToInsert, this,
            [this](PlaylistItem *parent, int row) {
                auto parentIndex = parent->parent() ? createIndex(parent->row(), 0, parent) : QModelIndex();
                beginInsertRows(parentIndex, row, row);
            });
    connect(m_playlistManager, &PlaylistManager::inserted, this,  &PlaylistModel::endInsertRows);

    connect(m_playlistManager, &PlaylistManager::aboutToRemove, this,
            [this](PlaylistItem *item) {
                auto isInvalid = item->parent() == m_playlistManager->root();
                auto parentIndex = !isInvalid ? createIndex(item->parent()->row(), 0, item->parent()) : QModelIndex();
                beginRemoveRows(parentIndex, item->row(), item->row());
            });
    connect(m_playlistManager, &PlaylistManager::removed, this,  &PlaylistModel::endRemoveRows);

    connect(m_playlistManager, &PlaylistManager::modelReset, this,
            [this]() {
                beginResetModel();
                endResetModel();
            });

    connect(m_playlistManager, &PlaylistManager::updateSelections, this,
            [this](PlaylistItem* currentItem) {
                auto itemIndex = currentItem ? createIndex(currentItem->row(), 0, currentItem) : QModelIndex();
                emit selectionsChanged(itemIndex);
            });

    connect(m_playlistManager, &PlaylistManager::dataChanged, this,
            [this](PlaylistItem *playlistItem) {
                auto index = createIndex(playlistItem->row(), 0, playlistItem);
                emit dataChanged(index, index);
            });

    connect(m_playlistManager, &PlaylistManager::scrollToCurrentIndex, this,
            [this]() {
                emit scrollToCurrentIndex();
            });


}

int PlaylistModel::rowCount(const QModelIndex &parent) const {
    if (parent.column() > 0) return 0;
    if (!parent.isValid()) {
        // top level: number of playlists in root
        return m_playlistManager->count();
    }
    auto parentItem = static_cast<PlaylistItem*>(parent.internalPointer());
    return parentItem ? parentItem->count() : 0;
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
        return item->type == PlaylistItem::LIST ? item->name : item->displayName;
    case IsCurrentIndexRole:
        if (!item->parent() || item->parent()->getCurrentIndex() == -1) return false;
        return item->parent()->getCurrentItem() == item;
    case IsDeletableRole:
        return item->count() > 0 || item->type == PlaylistItem::PASTED;
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

