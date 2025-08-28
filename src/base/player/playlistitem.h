#pragma once
#include <memory>
#include <atomic>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>
#include <QHash>
#include <QReadWriteLock>
#include <QElapsedTimer>

class ShowProvider;
class Video;
class PlaylistManager;
class PlaylistItem {
public:
    // Initialise as list
    PlaylistItem(const QString& name = "", ShowProvider* provider = nullptr, const QString &link = "") : name(name), m_provider(provider), link(link), type(LIST) {}
    // Initialise as item
    PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name, PlaylistItem *parent, bool isLocal = false);
    ~PlaylistItem();

    enum Type { LIST = 1, ONLINE = 2, LOCAL = 4, PASTED = 8 };

    QString name;
    QString displayName;
    QString link;
    int     season = 0;
    float   number = -1;
    int     type;

    std::unique_ptr<QFile> historyFile = nullptr;

    PlaylistItem *at(int i) const { return isValidIndex(i) ? m_children.at(i) : nullptr;       }
    PlaylistItem *first()   const { return at(0);                                              }
    PlaylistItem *last()    const { return at(count() - 1);                                    }
    PlaylistItem *parent()  const { return m_parent;                                           }
    bool isEmpty()          const { return m_children.isEmpty();                               }
    int  count()            const { return m_children.size();                                  }
    int  row()              const { return m_parent ? m_parent->m_children.indexOf(this) : -1; }

    bool          isValidIndex(int index) const;
    bool          setCurrentIndex(int index);

    int           getCurrentIndex() const { return m_currentIndex;     }
    PlaylistItem *getCurrentItem()  const { return at(m_currentIndex); }

    int  indexOf(const QString &link);
    int  indexOf(PlaylistItem *child)       const { return m_children.indexOf(child); }
    QListIterator<PlaylistItem*> iterator() const { return QListIterator<PlaylistItem*>(m_children); }
    bool isList()                           const { return type & Type::LIST; }
    bool isLocalDir()                       const { return type & (Type::LOCAL | Type::LIST); }

    void emplaceBack(int season, float number, const QString &link, const QString &name, bool isLocal = false);
    void append(PlaylistItem *value);
    void insert(int index, PlaylistItem* value);
    void removeAt(int index);
    void removeOne(PlaylistItem *value);
    void clear();
    void sort();
    void reverse();

    void   setTimestamp(qint64 timestamp);
    qint64 getTimestamp() const;
    void   updateHistoryFile();
    ShowProvider *getProvider() const { return m_provider; }

    void use();
    void disuse();
private:
    ShowProvider* m_provider;
    PlaylistItem *m_parent = nullptr;
    QList<PlaylistItem*> m_children;
    int m_currentIndex = -1;
    qint64 m_timestamp = 0;
    std::atomic<int> m_useCount = 0;
};
