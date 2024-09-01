#pragma once
#include "player/playlistitem.h"
#include <QAbstractListModel>
class EpisodeListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString continueText     READ getContinueText                         NOTIFY lastWatchedIndexChanged)
    Q_PROPERTY(int     lastWatchedIndex READ getLastWatchedIndex                     NOTIFY lastWatchedIndexChanged)
    Q_PROPERTY(bool    reversed         READ isReversed         WRITE setIsReversed  NOTIFY isReversedChanged)

public:
    explicit EpisodeListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {};
    ~EpisodeListModel() { delete m_rootItem; };

    void setPlaylist(PlaylistItem *playlist);
    void updateLastWatchedIndex();
    int getContinueIndex() const;



    bool isReversed() const { return m_isReversed; }
    void setIsReversed(bool isReversed) {
        if (m_isReversed == isReversed)
            return;
        m_isReversed = isReversed;
        emit layoutChanged();
        emit isReversedChanged();
    }
signals:
    void lastWatchedIndexChanged(void);
    void isReversedChanged(void);

private:
    int getLastWatchedIndex() const;
    PlaylistItem *m_rootItem = new PlaylistItem("root", nullptr, "/");
    int m_continueIndex = -1;
    QString m_continueText = "";
    QString getContinueText() const { return m_continueText; };
    bool m_isReversed = false;

    enum { TitleRole = Qt::UserRole, NumberRole, FullTitleRole };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};


