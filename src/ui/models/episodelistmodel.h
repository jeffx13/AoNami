#pragma once
#include <QAbstractListModel>
#include <memory>
#include <QSharedPointer>
#include <QWeakPointer>

class PlaylistItem;
class EpisodeListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool    reversed         READ isReversed          WRITE setIsReversed       NOTIFY isReversedChanged)

public:
    explicit EpisodeListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    ~EpisodeListModel() = default; // No need to manually clean up with shared pointers

    void setPlaylist(QSharedPointer<PlaylistItem> playlist);
    bool isReversed() const { return m_isReversed; }
    void setIsReversed(bool isReversed);
signals:
    void isReversedChanged(void);
private:
    QWeakPointer<PlaylistItem> m_playlist;
    bool m_isReversed = false;

    enum { 
        TitleRole = Qt::UserRole, 
        EpisodeNumberRole, 
        SeasonNumberRole,
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};


