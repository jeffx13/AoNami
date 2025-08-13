#pragma once

#include "myexception.h"
#include <QDir>
#include <memory>
#include <QMutex>
#include <QRegularExpression>
#include <QUrl>

class ShowProvider;
class Video;
class PlaylistManager;

class PlaylistItem {

public:
    // Initialise as list
    PlaylistItem(const QString& name, ShowProvider* provider, const QString &link) : name(name), m_provider(provider), link(link), type(LIST) {}

    // Initialise as item
    PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name, PlaylistItem *parent, bool isLocal = false);

    ~PlaylistItem() { clear(); }
    // qDebug() << "deleted" << (m_parent != nullptr ? m_parent->link : "") << fullName;

    enum Type { LIST, ONLINE, LOCAL, PASTED };
    Type type;
    QString name;
    QString displayName;
    QString link;
    int seasonNumber = 0;
    float number = -1;


    PlaylistItem *parent() const { return m_parent; }
    //QList<PlaylistItem*> *children() const { return m_children.get(); }
    QListIterator<PlaylistItem*> iterator() {
        if (isEmpty()) return QListIterator<PlaylistItem*>(QList<PlaylistItem*>());
        return QListIterator<PlaylistItem*>(*m_children.get());
    };


    PlaylistItem *at(int i) const { return !isValidIndex(i) ? nullptr : m_children->at(i); }
    PlaylistItem *first() const { return at(0); }
    PlaylistItem *last() const { return at(count() - 1); }
    PlaylistItem *getCurrentItem() const { return at(m_currentIndex); }

    int  row() { return m_parent ? m_parent->m_children->indexOf(const_cast<PlaylistItem *>(this)) : 0; }
    int  indexOf(PlaylistItem *child) { return m_children ? m_children->indexOf(child) : -1; }
    int  indexOf(const QString &link);
    bool isEmpty() const { return !m_children || m_children->size() == 0; }
    int  count() const { return m_children ? m_children->size() : 0; }

    int  getCurrentIndex() const { return m_currentIndex; }
    bool setCurrentIndex(int index) {
        if(index != -1 && !isValidIndex(index))
            return false;
        m_currentIndex = index;
        return true;
    }
    bool isValidIndex(int index) const;
    void reverse();

    void emplaceBack(int seasonNumber, float number, const QString &link, const QString &name, bool isLocal = false);
    void append(PlaylistItem *value);
    void insert(int index, PlaylistItem* value);
    void removeAt(int index);
    void removeOne(PlaylistItem *value);
    void removeLast() { if (m_children) removeAt(m_children->size() - 1); }
    void clear();

    void updateHistoryFile();

    inline ShowProvider *getProvider() const { return m_provider; }

    inline bool isLocalDir() const { return m_isLocalDir; }
    void setIsLocalDir(bool isLocalDir) {
        m_isLocalDir = isLocalDir;
    }

    void setTimestamp(qint64 timestamp) {
        if (type == LIST) {
            throw MyException("Cannot set timestamp for list");
        }
        m_timestamp = timestamp;
    }

    qint64 getTimestamp() const {
        if (type == LIST) {
            throw MyException("Cannot get timestamp for list");
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

    friend PlaylistManager;


private:

    ShowProvider* m_provider;
    bool m_isLocalDir = false;
    PlaylistItem *m_parent = nullptr;
    std::unique_ptr<QList<PlaylistItem*>> m_children = nullptr;

    std::unique_ptr<QFile> m_historyFile = nullptr;
    int m_currentIndex = -1;
    qint64 m_timestamp = 0;

    std::atomic<int> m_useCount = 0;

    void checkDelete(PlaylistItem *value);
    void createChildren();
};




