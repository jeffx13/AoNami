#pragma once
#include <QAbstractListModel>
class PlaylistItem;
class EpisodeListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool    reversed         READ isReversed          WRITE setIsReversed       NOTIFY isReversedChanged)

public:
    explicit EpisodeListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    ~EpisodeListModel() { setPlaylist(nullptr); }

    void setPlaylist(PlaylistItem *playlist);
    bool isReversed() const { return m_isReversed; }
    void setIsReversed(bool isReversed);
signals:
    void isReversedChanged(void);
private:
    PlaylistItem *m_playlist = nullptr;
    bool m_isReversed = false;

    enum { 
        TitleRole = Qt::UserRole, 
        EpisodeNumberRole, 
        SeasonNumberRole,
        FullTitleRole 
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};


