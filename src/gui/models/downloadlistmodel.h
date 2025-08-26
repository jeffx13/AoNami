#pragma once

#include <QAbstractListModel>

class DownloadManager;
class DownloadTask;

class DownloadListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit DownloadListModel(DownloadManager *manager);
    ~DownloadListModel() override = default;

    enum Role {
        NameRole = Qt::UserRole,
        PathRole,
        ProgressValueRole,
        ProgressTextRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    DownloadManager *m_manager;
};


