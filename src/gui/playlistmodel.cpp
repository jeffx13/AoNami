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
            [this](QModelIndex modelIndex) {
                beginRemoveRows(modelIndex.parent(), modelIndex.row(), modelIndex.row());
            });
    connect(m_playlistManager, &PlaylistManager::removed, this,  &PlaylistModel::endRemoveRows);

    connect(m_playlistManager, &PlaylistManager::modelReset, this,
            [this]() {
                beginResetModel();
                endResetModel();
            });

    connect(m_playlistManager, &PlaylistManager::currentIndexChanged, this,
            [this](int playlistIndex, int itemIndex, bool scrollToIndex) {
                if (playlistIndex == -1 || itemIndex == -1) {
                    emit selectionsChanged(QModelIndex(), false);
                    return;
                }
                auto parentIndex = index(playlistIndex, 0, QModelIndex());
                auto childIndex = index(itemIndex, 0, parentIndex);
                // auto childIndex = createIndex(itemIndex, 0, )
                emit selectionsChanged(childIndex, scrollToIndex);
            });

    connect(m_playlistManager, &PlaylistManager::changed, this,
            [this](int i) {
                emit dataChanged(index(i, 0, QModelIndex()), index(i, 0, QModelIndex()));
            });
}

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
        return item->type == PlaylistItem::LIST ? item->name : item->displayName;
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

