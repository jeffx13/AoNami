#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSet>
#include "core/network/network.h"
#include "core/playinfo.h"

class ShowProvider;

class ServerListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int currentIndex READ getCurrentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count        READ count                                NOTIFY countChanged)
    Q_PROPERTY(bool hasDub      READ hasDub                               NOTIFY countChanged)
public:
    // Per-server state; broken servers stay visible (greyed) rather than vanish.
    enum Status { Unchecked, Working, Broken };

    ServerListModel() = default;
    ~ServerListModel() = default;

    void setServers(const QList<VideoServer> &servers, ShowProvider *provider);
    void setCurrentIndex(int index);
    void setCurrentServer(const QString &name);   // set current + preferred by name (sort-safe)
    void setPreferredServer(int index);
    void clear();

    VideoServer& at(int index);
    int count() const { return m_servers.count(); }
    bool hasDub() const;
    int getCurrentIndex() const { return m_currentIndex; }
    bool isValidIndex(int index) const;

    ShowProvider *provider() const { return m_provider; }

    // Extracted PlayInfo cache keyed by server name (a cached server = Working).
    void setCachedSources(QHash<QString, PlayInfo> &&cache);
    void cacheSource(const QString &name, PlayInfo info);
    void markBroken(const QString &name);
    const PlayInfo *cachedSource(const QString &name) const { auto it = m_sourceCache.constFind(name); return it != m_sourceCache.constEnd() ? &*it : nullptr; }

signals:
    void currentIndexChanged();
    void countChanged();

private:
    int m_currentIndex = -1;
    QList<VideoServer> m_servers;
    ShowProvider *m_provider = nullptr;
    QHash<QString, PlayInfo> m_sourceCache;
    QSet<QString> m_brokenServers;

    void emitStatusChanged(const QString &name);
    void resort();   // keep working servers (grouped by translation) first, broken last

    enum { NameRole = Qt::UserRole, LinkRole, StatusRole, TranslationRole, SectionRole };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
};
