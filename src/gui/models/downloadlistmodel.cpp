#include "downloadlistmodel.h"
#include "base/downloadmanager.h"

DownloadListModel::DownloadListModel(DownloadManager *manager)
    : m_manager(manager)
{
    QObject::connect(m_manager, &DownloadManager::aboutToInsert, this, [this](int row) {
        beginInsertRows(QModelIndex(), row, row);
    });
    QObject::connect(m_manager, &DownloadManager::inserted, this, &DownloadListModel::endInsertRows);

    QObject::connect(m_manager, &DownloadManager::aboutToRemove, this, [this](int row) {
        beginRemoveRows(QModelIndex(), row, row);
    });
    QObject::connect(m_manager, &DownloadManager::removed, this, &DownloadListModel::endRemoveRows);

    QObject::connect(m_manager, &DownloadManager::dataChanged, this, [this](int row) {
        auto idx = createIndex(row, 0);
        emit dataChanged(idx, idx, {});
    });

    QObject::connect(m_manager, &DownloadManager::modelReset, this, [this]() {
        beginResetModel();
        endResetModel();
    });
}

int DownloadListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_manager->count();
}

QVariant DownloadListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    auto task = m_manager->taskAt(index.row());
    if (!task) return QVariant();
    switch (role) {
    case NameRole:
        return task->displayName;
    case PathRole:
        return task->path;
    case ProgressValueRole:
        return task->getProgressValue();
    case ProgressTextRole:
        return task->getProgressText();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> DownloadListModel::roleNames() const {
    QHash<int, QByteArray> names;
    names[NameRole] = "downloadName";
    names[PathRole] = "downloadPath";
    names[ProgressValueRole] = "progressValue";
    names[ProgressTextRole] = "progressText";
    return names;
}


