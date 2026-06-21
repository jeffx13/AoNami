#pragma once
#include <QAbstractListModel>
#include "core/playinfo.h"

// Two keys per track: Index (position here) and ID (mpv's gappy track id).
class TrackListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count        READ count                                NOTIFY countChanged)
public:
    TrackListModel() = default;
    ~TrackListModel() = default;

    Track *at(int index) {
        return isValidIndex(index) ? &m_tracks[index] : nullptr;
    }

    int count() const { return m_tracks.count(); }
    bool isValidIndex(int index) const { return index >= 0 && index < m_tracks.size(); }

    int getCurrentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index) {
        if (index == m_currentIndex || !isValidIndex(index)) return;
        m_currentIndex = index;
        emit currentIndexChanged();
    }
    void setCurrentId(int64_t id) {
        int idx = indexForId(id);
        if (idx >= 0) setCurrentIndex(idx);
    }

    [[nodiscard]] int64_t idForIndex(int index) const { return m_indexToId.value(index, -1); }
    [[nodiscard]] int indexForId(int64_t id) const { return m_idToIndex.value(id, -1); }

    void setId(const QUrl &url, int64_t id) {
        int idx = indexOf(url);
        if (idx < 0) return;
        int64_t oldId = m_indexToId.value(idx, -1);
        if (oldId != -1 && oldId != id) m_idToIndex.remove(oldId);
        m_indexToId[idx] = id;
        m_idToIndex[id] = idx;
    }

    bool hasTitle(int64_t id) const {
        int idx = indexForId(id);
        return idx >= 0 && !m_tracks[idx].title.isEmpty();
    }

    // Append an external track by URL. Returns false if already present.
    bool append(const QUrl &url, const QString &title, const QString &lang = "") {
        if (indexOf(url) >= 0) return false;
        int row = m_tracks.size();
        beginInsertRows(QModelIndex(), row, row);
        m_tracks.append(Track(url, title, lang));
        int64_t syntheticId = row + 1;  // Provisional; overwritten by setId when mpv reports it
        m_indexToId[row] = syntheticId;
        m_idToIndex[syntheticId] = row;
        m_urlToIndex[url] = row;
        endInsertRows();
        emit countChanged();
        return true;
    }

    // Append a track by mpv ID (no URL - internal mpv track)
    void append(int64_t id, const QString &title, const QString &lang = "") {
        int row = m_tracks.size();
        beginInsertRows(QModelIndex(), row, row);
        m_tracks.append(Track(QUrl(), title, lang));
        m_indexToId[row] = id;
        m_idToIndex[id] = row;
        endInsertRows();
        emit countChanged();
    }

    void updateById(int64_t id, const QString &title) {
        int idx = indexForId(id);
        if (idx < 0) return;
        m_tracks[idx].title = title;
        auto modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx);
    }

    void clear() {
        beginResetModel();
        m_tracks.clear();
        m_urlToIndex.clear();
        m_indexToId.clear();
        m_idToIndex.clear();
        m_currentIndex = -1;
        endResetModel();
        emit currentIndexChanged();
        emit countChanged();
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return count(); }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || !isValidIndex(index.row())) return {};
        if (role == Qt::DisplayRole) return m_tracks.at(index.row()).title;
        return {};
    }

    QHash<int, QByteArray> roleNames() const override {
        return {{Qt::DisplayRole, "name"}};
    }

signals:
    void currentIndexChanged();
    void countChanged();

private:
    int indexOf(const QUrl &url) const {
        return url.isEmpty() ? -1 : m_urlToIndex.value(url, -1);
    }

    QList<Track> m_tracks;
    int m_currentIndex = -1;

    // ID and index lookups, both directions
    QMap<int, int64_t> m_indexToId;   // model index -> mpv track ID
    QMap<int64_t, int> m_idToIndex;   // mpv track ID -> model index
    QMap<QUrl, int>    m_urlToIndex;  // URL -> model index (for dedup + setId lookup)
};
