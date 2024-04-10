#pragma once
#include "data/playlistitem.h"
#include <QAbstractListModel>
class EpisodeListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString continueText     READ getContinueText                         NOTIFY continueIndexChanged)
    Q_PROPERTY(int     continueIndex    READ getContinueIndex                        NOTIFY continueIndexChanged)
    Q_PROPERTY(int     lastWatchedIndex READ getLastWatchedIndex                     NOTIFY continueIndexChanged)
    Q_PROPERTY(bool    reversed         READ getIsReversed       WRITE setIsReversed NOTIFY reversedChanged)

public:
    explicit EpisodeListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {};
    ~EpisodeListModel() { delete m_rootItem; };

    void setPlaylist(PlaylistItem *playlist);
    void updateLastWatchedIndex();
    int getContinueIndex() const;
    int correctIndex(int index) const;
signals:
    void continueIndexChanged(void);
    void reversedChanged(void);

private:
    int getLastWatchedIndex() const;
    PlaylistItem *m_rootItem = new PlaylistItem("root", nullptr, "/");
    int m_continueIndex = -1;
    QString m_continueText = "";
    QString getContinueText() const { return m_continueText; };

    bool m_isReversed = false;
    bool getIsReversed() const { return m_isReversed; }
    void setIsReversed(bool isReversed) {
        if (m_isReversed == isReversed)
            return;
        m_isReversed = isReversed;
        emit layoutChanged();
        emit reversedChanged();
    }

    enum { TitleRole = Qt::UserRole, NumberRole, FullTitleRole };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};


