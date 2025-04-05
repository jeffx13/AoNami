#pragma once
#include "player/playlistitem.h"

#include <QAbstractListModel>



class VideoListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count        READ getCount                              NOTIFY countChanged)
public:
    VideoListModel() {}
    ~VideoListModel() = default;

    void setVideos(const QList<Video> *videos) {
        m_videos = videos;
        m_currentIndex = 0;
        emit currentIndexChanged();
        emit layoutChanged();
        emit countChanged();
    }
    int getCount() const {
        if (!m_videos) return 0;
        return m_videos->size();
    }
    void setCurrentIndex(int index) {
        if (index == m_currentIndex || !isValidIndex(index)) return;
        m_currentIndex = index;
        emit currentIndexChanged();
    }
    int getCurrentIndex() const { return m_currentIndex; }

    bool isValidIndex(int index) const {
        if (!m_videos) return false;
        return index >= 0 && index < m_videos->size();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (!m_videos) return 0;
        return m_videos->size();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || !m_videos)
            return QVariant();

        const Video &video = m_videos->at(index.row());
        return video.label;
    }
    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles[Qt::DisplayRole] = "label";
        return roles;
    }
signals:
    void currentIndexChanged();
    void countChanged();
private:
    const QList<Video> *m_videos = nullptr;
    int m_currentIndex = -1;



};
