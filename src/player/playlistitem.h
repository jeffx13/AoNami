#pragma once
#include "network/myexception.h"
#include <QDir>
#include <memory>
#include <QMutex>
#include <QRegularExpression>
#include <QUrl>

class ShowProvider;
class Video;

class PlaylistItem {
public:
    //List
    PlaylistItem(const QString& name, ShowProvider* provider, const QString &link)
        : name(name), m_provider(provider), link(link), type(LIST) {
    }

    static PlaylistItem *fromLocalUrl(const QUrl &pathUrl);

    //Item
    PlaylistItem(int seasonNumber, float number, const QString &link, const QString &name, PlaylistItem *parent, bool isLocal = false);
    ~PlaylistItem() {
        clear();
        // qDebug() << "deleted" << (m_parent != nullptr ? m_parent->link : "") << fullName;
    }

    inline bool reloadFromFolder() { return loadFromFolder (QUrl()); }

    inline PlaylistItem *parent() const { return m_parent; }
    inline PlaylistItem *at(int i) const { return !isValidIndex(i) ? nullptr : m_children->at(i); }
    inline int row() { return m_parent ? m_parent->m_children->indexOf(const_cast<PlaylistItem *>(this)) : 0; }
    inline PlaylistItem *first() const { return at(0); }
    inline PlaylistItem *last() const { return at(size() - 1); }
    inline PlaylistItem *getCurrentItem() const { return at(currentIndex); }
    inline int indexOf(PlaylistItem *child) { return m_children ? m_children->indexOf (child) : -1; }


    int indexOf(const QString &link);
    inline bool isEmpty() const { return !m_children || m_children->size() == 0; }
    inline int size() const { return m_children ? m_children->size() : 0; }
    bool isValidIndex(int index) const;

    void append(PlaylistItem *value);
    void removeOne(PlaylistItem *value) {
        if (!m_children || !value) return;
        m_children->removeOne(value);
        checkDelete(value);

    }



    void reverse() {
        if (!m_children || m_children->isEmpty()) return;
        std::reverse(m_children->begin(), m_children->end());
    }
    void emplaceBack(int seasonNumber, float number, const QString &link, const QString &name, bool isLocal = false);
    void clear();
    void removeAt(int index);
    void insert(int index, PlaylistItem* value) {
        if (index == 0 || isValidIndex(index)) {
            createChildren();
            value->useCount++;
            value->m_parent = this;
            m_children->insert(index, value);
        }
    }
    QList<PlaylistItem*> *children() const { return m_children.get(); }
    inline void removeLast() { if (m_children) removeAt(m_children->size() - 1); }
    bool replace(int index, PlaylistItem *value);



    QString getDisplayNameAt(int index) const;
    void updateHistoryFile(qint64 time = 0);
    void setLastPlayAt(int index, int time);
    inline bool isLoadedFromFolder() const { return m_isLoadedFromFolder; }

    inline ShowProvider *getProvider() const { return m_provider; }
    inline QString getFullName() const { return fullName; }

    enum Type { LIST, ONLINE, LOCAL };
    Type type;

    QString name;
    int seasonNumber = 0;
    float number = -1;
    QString link;
    int currentIndex = -1;
    int timeStamp = 0;
    void use(){
        ++useCount;
        // qDebug() << "use" << useCount << (m_parent != nullptr ? m_parent->link : "") << fullName;;
    }
    void disuse() {
        // qDebug() << "disused" << useCount << (m_parent != nullptr ? m_parent->link : "") << fullName;;
        if (--useCount == 0) {
            delete this;
        }
    }
private:
    QString fullName;
    ShowProvider* m_provider;
    bool m_isLoadedFromFolder = false;
    std::unique_ptr<QFile> m_historyFile = nullptr;
    inline static QRegularExpression fileNameRegex{ R"((?:Episode|Ep\.?)?\s*(?<number>\d+\.?\d*)?\s*[\.:]?\s*(?<title>.*)?\.\w{3})" };
    PlaylistItem *m_parent = nullptr;
    std::unique_ptr<QList<PlaylistItem*>> m_children = nullptr;
    std::atomic<int> useCount = 0;
    void checkDelete(PlaylistItem *value) {
        value->m_parent = nullptr;
        if (--value->useCount == 0) {
            delete value;
        }
    }
    void createChildren() {
        if (m_children) return;
        m_children = std::unique_ptr<QList<PlaylistItem*>>(new QList<PlaylistItem*>);
    }
    bool loadFromFolder(const QUrl &pathUrl);
};




