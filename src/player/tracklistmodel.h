#pragma once
#include "playinfo.h"

#include <QAbstractListModel>


class TrackListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count        READ count                              NOTIFY countChanged)
public:
    TrackListModel() {}
    ~TrackListModel() = default;
    QList<Track> &list() {
        return m_data;
    }

    bool hasTitleById(int id) {
        if (!m_idToIndex.contains(id)) {
            gLog() << "Track" << "id doesnt exist" << id;
            return false;
        }
        return !m_data[m_idToIndex[id]].title.isEmpty();
    }

    void append(const QUrl url, const QString &title, const QString &lang = "") {
        beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
        m_data.append(Track(url, title, lang));
        endInsertRows();

        m_urlToIndex[url] = m_data.size() - 1;
        m_indexToId[m_data.size() - 1] = m_data.size();
        m_idToIndex[m_data.size()] = m_data.size() - 1;

        emit countChanged();
    }

    void append(const QString &title, const QString &lang = "") {
        beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
        m_data.append(Track(QUrl(), title, lang));
        endInsertRows();

        m_indexToId[m_data.size()] = m_data.size() + 1;
        m_idToIndex[m_data.size() + 1] = m_data.size();

        emit countChanged();
    }

    int64_t getId(int index) {
        return m_indexToId.value(index, -1);
    }

    void updateById(int id, const QString &title) {
        if (id <= 0 || id > m_data.size()) return;
        int index = m_idToIndex.value(id, -1);
        if (index == -1) return;
        m_data[index].title = title;
        emit dataChanged(createIndex(index, 0), createIndex(index, 0));
    }

    void setId(const QUrl &url, int aid) {
        if (!m_urlToIndex.contains(url)) {
            return;
        }
        int index = m_urlToIndex[url];
        m_indexToId[index] = aid;
        m_idToIndex[aid] = index;
    }

    int count() const {
        return m_data.count();
    }
    void setCurrentIndex(int index) {
        if (index == m_currentIndex ||!isValidIndex(index)) return;
        m_currentIndex = index;
        emit currentIndexChanged();
    }

    void setCurrentIndexById(int64_t id) {
        int index = m_idToIndex.value(id, -1);
        if (index != -1) {
            setCurrentIndex(index);
        }
    }

    int getCurrentIndex() const { return m_currentIndex; }

    bool isValidIndex(int index) const {
        return index >= 0 && index < m_data.size();
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        return m_data.size();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid())
            return QVariant();
        const Track &audio = m_data.at(index.row());
        return audio.title;
    }

    void clear() {
        beginResetModel();
        m_data.clear();
        endResetModel();

        m_currentIndex = -1;
        m_urlToIndex.clear();
        m_indexToId.clear();
        emit currentIndexChanged();
        emit countChanged();
    }
    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles[Qt::DisplayRole] = "title";
        return roles;
    }
signals:
    void currentIndexChanged();
    void countChanged();
private:
    QList<Track> m_data;
    int m_currentIndex = -1;
    QMap<QUrl, int> m_urlToIndex;
    QMap<int, int64_t> m_indexToId;
    QMap<int64_t, int> m_idToIndex;

};
