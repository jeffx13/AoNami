#pragma once
#include "playinfo.h"

#include <QAbstractListModel>


class AudioListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count        READ getCount                              NOTIFY countChanged)
public:
    AudioListModel() {}
    ~AudioListModel() = default;

    void setAudios(const QList<AudioTrack> *audios) {
        m_audios = audios;
        m_currentIndex = 0;
        emit currentIndexChanged();
        emit countChanged();
        emit layoutChanged();
    }
    int getCount() const {
        if (!m_audios) return 0;
        return m_audios->size();
    }
    void setCurrentIndex(int index) {
        if (index == m_currentIndex || !isValidIndex(index)) return;
        m_currentIndex = index;
        emit currentIndexChanged();
    }
    int getCurrentIndex() const { return m_currentIndex; }
    bool isValidIndex(int index) const {
        if (!m_audios) return false;
        return index >= 0 && index < m_audios->size();
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (!m_audios) return 0;
        return m_audios->size();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || !m_audios)
            return QVariant();
        const AudioTrack &audio = m_audios->at(index.row());
        return audio.label;
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
    const QList<AudioTrack> *m_audios = nullptr;
    int m_currentIndex = -1;

};
