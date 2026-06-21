#pragma once
#include <QSharedPointer>
#include <QWeakPointer>
#include <QEnableSharedFromThis>
#include <QScopedPointer>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>
#include <QHash>

class ShowProvider;
class Video;
class PlaylistManager;

class PlaylistItem : public QEnableSharedFromThis<PlaylistItem> {
public:
    enum Type { LIST = 1, ONLINE = 2, LOCAL = 4, PASTED = 8 };

    // Create a list (playlist container)
    PlaylistItem(const QString& name = "", ShowProvider* provider = nullptr, const QString &link = "");
    // Create an item (episode/track)
    PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name,
                 QSharedPointer<PlaylistItem> parent, bool isLocal = false);
    ~PlaylistItem();

    // Non-copyable, non-movable (shared_ptr managed)
    PlaylistItem(const PlaylistItem&) = delete;
    PlaylistItem& operator=(const PlaylistItem&) = delete;

    QString name;
    QString displayName;
    QString link;
    int     season = 0;
    float   number = -1;
    int     type;

    bool isList()     const { return type & Type::LIST; }
    bool isLocalDir() const { return (type & Type::LOCAL) && (type & Type::LIST); }

    QSharedPointer<PlaylistItem> at(int i)     const { return isValidIndex(i) ? m_children.at(i) : nullptr; }
    QSharedPointer<PlaylistItem> first()       const { return at(0); }
    QSharedPointer<PlaylistItem> last()        const { return at(count() - 1); }
    QSharedPointer<PlaylistItem> parent()      const { return m_parent.toStrongRef(); }
    bool  isEmpty()       const { return m_children.isEmpty(); }
    int   count()         const { return m_children.size(); }
    int   row()           const { return m_row; }
    bool  isValidIndex(int index) const;
    int   indexOf(const QString &link);
    int   indexOf(QSharedPointer<PlaylistItem> child) const { return m_children.indexOf(child); }
    QListIterator<QSharedPointer<PlaylistItem>> iterator() { return QListIterator<QSharedPointer<PlaylistItem>>(m_children); }

    void emplaceBack(int season, float number, const QString &link, const QString &name, bool isLocal = false);
    void append(QSharedPointer<PlaylistItem> value);
    void insert(int index, QSharedPointer<PlaylistItem> value);
    void removeAt(int index);
    void removeOne(QSharedPointer<PlaylistItem> value);
    void reserve(int n) { m_children.reserve(n); }
    void clear();
    void sort();
    void reverse();

    bool setCurrentIndex(int index);
    int  getCurrentIndex() const { return m_currentIndex; }
    QSharedPointer<PlaylistItem> getCurrentItem() const { return at(m_currentIndex); }

    void   setTimestamp(qint64 timestamp);
    qint64 getTimestamp() const;

    // Only meaningful for LIST nodes
    ShowProvider *getProvider() const { return m_provider; }

    // Only used for LOCAL|LIST nodes to remember last-played file
    QScopedPointer<QFile> historyFile;
    void updateHistoryFile();

private:
    void updateRowIndices(int startIndex = 0);

    ShowProvider* m_provider = nullptr;
    QWeakPointer<PlaylistItem> m_parent;
    QList<QSharedPointer<PlaylistItem>> m_children;
    int m_currentIndex = -1;
    qint64 m_timestamp = 0;
    int m_row = -1;
};
