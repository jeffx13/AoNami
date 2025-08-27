#pragma once

#include "app/appexception.h"
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

    ~PlaylistItem() {
        clear();
        if (m_parent) {
            m_parent->m_children.removeOne(this);
        }
    }
    // qDebug() << "deleted" << (m_parent != nullptr ? m_parent->link : "") << fullName;

    enum Type { LIST, ONLINE, LOCAL, PASTED };
    Type type;
    QString name;
    QString displayName;
    QString link;
    int seasonNumber = 0;
    float number = -1;
    std::unique_ptr<QFile> historyFile = nullptr;


    PlaylistItem *parent() const { return m_parent; }

    QListIterator<PlaylistItem*> iterator() {
        if (isEmpty()) return QListIterator<PlaylistItem*>(QList<PlaylistItem*>());
        return QListIterator<PlaylistItem*>(m_children);
    };


    PlaylistItem *at(int i) const { return !isValidIndex(i) ? nullptr : m_children.at(i); }
    PlaylistItem *first() const { return at(0); }
    PlaylistItem *last() const { return at(count() - 1); }
    PlaylistItem *getCurrentItem() const { return at(m_currentIndex); }

    int  row() { return m_parent ? m_parent->m_children.indexOf(const_cast<PlaylistItem *>(this)) : 0; }
    int  indexOf(PlaylistItem *child) { return m_children.indexOf(child); }
    int  indexOf(const QString &link);
    bool isEmpty() const { return m_children.isEmpty(); }
    int  count() const { return m_children.size(); }

    int  getCurrentIndex() const { return m_currentIndex; }
    bool setCurrentIndex(int index);
    bool isValidIndex(int index) const;
    void reverse();

    void emplaceBack(int seasonNumber, float number, const QString &link, const QString &name, bool isLocal = false);
    void append(PlaylistItem *value);
    void insert(int index, PlaylistItem* value);
    void removeAt(int index);
    void removeOne(PlaylistItem *value);
    void removeLast() { if (!m_children.isEmpty()) removeAt(m_children.size() - 1); }
    void clear();
    void sort();

    void updateHistoryFile();

    inline ShowProvider *getProvider() const { return m_provider; }

    inline bool isLocalDir() const { return m_isLocalDir; }
    void setIsLocalDir(bool isLocalDir) { m_isLocalDir = isLocalDir; }
    bool isList() const { return type == LIST; }

    void setTimestamp(qint64 timestamp) {
        if (type == LIST) {
            throw AppException("Cannot set timestamp for list");
        }
        m_timestamp = timestamp;
    }

    qint64 getTimestamp() const {
        if (type == LIST) {
            throw AppException("Cannot get timestamp for list");
        }
        return m_timestamp;
    }

    void use(){
        ++m_useCount;
        // qDebug() << "use" << useCount << (m_parent != nullptr ? m_parent->link : "") << fullName;;
    }
    void disuse() {
        // qDebug() << "disused" << useCount << (m_parent != nullptr ? m_parent->link : "") << fullName;;
        if (--m_useCount == 0) {
            delete this;
        }
    }
private:

    ShowProvider* m_provider;
    bool m_isLocalDir = false;
    PlaylistItem *m_parent = nullptr;
    QList<PlaylistItem*> m_children;

    int m_currentIndex = -1;
    qint64 m_timestamp = 0;

    std::atomic<int> m_useCount = 0;
    void checkDelete(PlaylistItem *value);
};




