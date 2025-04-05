#pragma once

#include "player/playinfo.h"
#include <QAbstractListModel>

class SubtitleListModel : public QAbstractListModel {

    Q_OBJECT
    Q_PROPERTY(int currentIndex     READ getCurrentIndex        WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QUrl currentSubtitle READ getCurrentSubtitleFile                       NOTIFY currentIndexChanged)
    Q_PROPERTY(int count            READ rowCount                                     NOTIFY layoutChanged)
public:
    SubtitleListModel() = default;
    ~SubtitleListModel() = default;

    void setCurrentIndex(int index) {
        if (index == m_currentIndex) return;
        m_currentIndex = index;
        emit currentIndexChanged();
    }

    void setSubtitles(const QList<SubTrack> *subtitles) {
        m_subtitles = subtitles;
        if (subtitles->isEmpty()) {
            setCurrentIndex(-1);
        } else {
            for (int i = 0; i < m_subtitles->size(); i++) {
                auto labelName= m_subtitles->at(i).label.toLower();
                if (labelName.contains("english")) {
                    setCurrentIndex(i);
                    break;
                }
            }

        }
        emit layoutChanged();
    }

    QUrl getCurrentSubtitleFile() const {
        if (!m_subtitles || m_currentIndex < 0 || m_currentIndex >= m_subtitles->size()) return QUrl();
        return m_subtitles->at(m_currentIndex).filePath;
    }

    int getCurrentIndex() const { return m_currentIndex; }

    enum {
        LabelRole = Qt::UserRole,
        FileRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (!m_subtitles) return 0;
        return m_subtitles->size();
    };
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || !m_subtitles)
            return QVariant();

        const SubTrack &subtitle = m_subtitles->at(index.row());
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
signals:
    void currentIndexChanged();
private:
    const QList<SubTrack> *m_subtitles = nullptr;
    int m_currentIndex = -1;


};



