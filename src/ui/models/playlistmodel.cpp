#include "playlistmodel.h"
#include "playlistitem.h"

PlaylistModel::PlaylistModel(PlaylistManager *playlistManager) : m_playlistManager(playlistManager) {
    connect(m_playlistManager, &PlaylistManager::aboutToInsert, this,
            [this](PlaylistItem* parent, int row) {
                auto parentIndex = (parent == m_playlistManager->root().data())
                    ? QModelIndex()
                    : createIndex(parent->row(), 0, parent);
                beginInsertRows(parentIndex, row, row);
            });
    connect(m_playlistManager, &PlaylistManager::inserted, this,  &PlaylistModel::endInsertRows);

    connect(m_playlistManager, &PlaylistManager::aboutToRemove, this,
            [this](PlaylistItem* item) {
                auto parent = item->parent();
                auto isInvalid = parent == m_playlistManager->root();
                auto parentIndex = !isInvalid ? createIndex(parent->row(), 0, parent.get()) : QModelIndex();
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
            [this](PlaylistItem* playlistItem) {
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
    PlaylistItem *parentItem = parent.isValid() ? static_cast<PlaylistItem*>(parent.internalPointer()) : m_playlistManager->root().data();
    return parentItem->count();
}

QModelIndex PlaylistModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();

    PlaylistItem *parentItem = parent.isValid() ? static_cast<PlaylistItem*>(parent.internalPointer()) : m_playlistManager->root().data();
    auto childItem = parentItem->at(row).data();
    return childItem ? createIndex(row, column, childItem) : QModelIndex();
}

QModelIndex PlaylistModel::parent(const QModelIndex &childIndex) const {
    if (!childIndex.isValid()) return QModelIndex();
    PlaylistItem *childItem = static_cast<PlaylistItem*>(childIndex.internalPointer());
    auto parentItem = childItem ? childItem->parent() : nullptr;
    if (!parentItem || parentItem.get() == m_playlistManager->root().data()) return QModelIndex();
    return createIndex(parentItem->row(), 0, parentItem.data());
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();

    auto item = static_cast<PlaylistItem*>(index.internalPointer());
    if (!item) return QVariant();
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole: return item->name;
    case IndexRole: return index;
    case NumberRole:
        return item->number;
    case NumberTitleRole:
        return item->isList() ? item->name : item->displayName;
    case IsCurrentIndexRole: {
        auto parent = item->parent();
        if (!parent || parent->getCurrentIndex() == -1) return false;
        return parent->getCurrentItem().data() == item;
    }
    case IsDeletableRole:
        return (item->count() > 0) || ((item->type & PlaylistItem::PASTED) != 0);
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
            // {RowRole, "row"},
            // {IdRole, "id"},
            {NumberTitleRole, "numberTitle"},
            {IsCurrentIndexRole, "isCurrentIndex"},
            {IsDeletableRole, "isDeletable"}
        };
    return names;
}
