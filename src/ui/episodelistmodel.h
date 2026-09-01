#pragma once
#include <QAbstractListModel>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QVector>

class PlaylistItem;

class EpisodeListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool    reversed   READ isReversed  WRITE setIsReversed  NOTIFY isReversedChanged)
    Q_PROPERTY(QString filterText READ filterText  WRITE setFilterText  NOTIFY filterTextChanged)

public:
    explicit EpisodeListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    ~EpisodeListModel() = default;

    void setPlaylist(const QSharedPointer<PlaylistItem> &playlist);
    bool isReversed() const { return m_isReversed; }
    void setIsReversed(bool isReversed);
    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &text);

    // Map visual row -> source index (accounts for both filter and reversal)
    Q_INVOKABLE int sourceIndex(int visibleRow) const;
    // Map source index -> visual row (-1 if filtered out)
    Q_INVOKABLE int visibleIndex(int sourceIdx) const;

signals:
    void isReversedChanged();
    void filterTextChanged();

private:
    QWeakPointer<PlaylistItem> m_playlist;
    bool    m_isReversed = false;
    QString m_filterText;
    QVector<int> m_filteredIndices; // source indices of matching episodes (forward order)

    void rebuildFilteredIndices();

    enum {
        TitleRole = Qt::UserRole,
        EpisodeNumberRole,
        SeasonNumberRole,
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};
