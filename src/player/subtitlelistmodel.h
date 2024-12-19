#pragma once

#include "player/playinfo.h"
#include <QAbstractListModel>

class SubtitleListModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QUrl currentSubtitle READ getCurrentSubtitleFile NOTIFY currentIndexChanged)
public:
    SubtitleListModel() = default;
    ~SubtitleListModel() = default;

    void setCurrentIndex(int index) {
        if (index == m_currentIndex) return;
        m_currentIndex = index;
        emit currentIndexChanged();
    }

    void setList(QVector<SubTrack> subtitles) {
        m_subtitles = subtitles;
        if (m_subtitles.isEmpty() || m_subtitles.first().label != "English") {
            setCurrentIndex(-1);
        } else {
            setCurrentIndex(0);
        }
        emit layoutChanged();
    }

    QUrl getCurrentSubtitleFile() const {
        if (m_currentIndex < 0 && m_currentIndex >= m_subtitles.size()) return QUrl();
        return m_subtitles.at(m_currentIndex).filePath;
    }

    int getCurrentIndex() const { return m_currentIndex; }

    QList<SubTrack> subtitles() const { return m_subtitles; }

    bool isEmpty() const { return m_subtitles.isEmpty(); }

    void clear() {
        m_subtitles.clear();
        setCurrentIndex(-1);
        emit layoutChanged();
    }

    int size() const { return m_subtitles.size(); }
signals:
    void currentIndexChanged();
private:
    QList<SubTrack> m_subtitles;
    int m_currentIndex = -1;

    enum {
        LabelRole = Qt::UserRole,
        FileRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return m_subtitles.size();
    };
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || m_subtitles.isEmpty())
            return QVariant();

        const SubTrack &subtitle = m_subtitles.at(index.row());
        switch (role) {
        case LabelRole:
            return subtitle.label;
            break;
        case FileRole:
            return subtitle.filePath;
            break;
        default:
            break;
        }
        return QVariant();
    };
    QHash<int, QByteArray> roleNames() const override{
        QHash<int, QByteArray> names;
        names[LabelRole] = "label";
        names[FileRole] = "file";
        return names;
    };
};



