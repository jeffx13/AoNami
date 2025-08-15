#pragma once
#include <QDir>
#include <QFileSystemWatcher>
#include <QStandardItemModel>
#include "base/showdata.h"
#include "gui/serverlistmodel.h"
#include "playlistitem.h"
#include "base/servicemanager.h"

class PlaylistManager : public ServiceManager {
    Q_OBJECT
    Q_PROPERTY(ServerListModel *serverList READ getServerList CONSTANT)
public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager() { }

    PlaylistItem *root() const { return m_root.get(); }
    PlaylistItem *find(const QString &link);
    int count() const { return m_root->count(); }

    int append(PlaylistItem *playlist, PlaylistItem *parent = nullptr) { return insert(INT_MAX, playlist, parent); }
    int insert(int index, PlaylistItem *playlist, PlaylistItem *parent = nullptr);
    int replace(int index, PlaylistItem *playlist, PlaylistItem *parent = nullptr);
    Q_INVOKABLE void remove(QModelIndex index);
    Q_INVOKABLE void clear();

    //  Traversing the playlist
    Q_INVOKABLE bool tryPlay(int playlistIndex = -1, int itemIndex = -1);
    bool tryPlay(PlaylistItem *item);

    Q_INVOKABLE void loadNextItem(int offset = 1);
    Q_INVOKABLE void loadNextPlaylist(int offset = 1);
    Q_INVOKABLE void loadIndex(const QModelIndex &index);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void openUrl(QUrl url, bool play);
    Q_INVOKABLE void loadServer(int index);

    Q_INVOKABLE void showCurrentItemName() const;
    Q_INVOKABLE void saveProgress() const;

    // void updateSelection(bool scrollToIndex = false);
    Q_INVOKABLE void cancel();

    // Signals
    Q_SIGNAL void aboutToInsert(PlaylistItem *parent, int index);
    Q_SIGNAL void inserted();
    Q_SIGNAL void aboutToRemove(PlaylistItem *item);
    Q_SIGNAL void removed();
    Q_SIGNAL void changed(int playlistIndex);
    Q_SIGNAL void updateSelections(PlaylistItem *currentItem, bool scrollToIndex = false); //int playlistIndex, int itemIndex,
    Q_SIGNAL void modelReset();

    Q_SIGNAL void progressUpdated(QString link, int progressIndex, int timestamp) const;
    Q_SIGNAL void aboutToPlay(void) const;
private:
    std::unique_ptr<PlaylistItem> m_root = std::unique_ptr<PlaylistItem>(new PlaylistItem("root", nullptr, "/"));
    Client m_client = Client(&m_isCancelled);

    PlayInfo play(PlaylistItem *item);

    PlaylistItem *m_currentItem = nullptr;
    void setCurrentItem(PlaylistItem *currentItem);

    // Watchers
    QFileSystemWatcher m_folderWatcher;
    QFutureWatcher<PlayInfo> m_watcher;

    // models
    ServerListModel m_serverListModel;
    ServerListModel *getServerList() { return &m_serverListModel; }

    // Prevents the playlist with the same link being added
    QMap<QString, PlaylistItem*> m_playlistMap;
    void registerPlaylist(PlaylistItem *playlist);
    void deregisterPlaylist(PlaylistItem *playlist);
    QStringList m_playableExtensions {"*.mp4", "*.mkv", "*.avi", "*.mp3", "*.flac", "*.wav", "*.ogg", "*.webm", "*.m3u8", "*.mov"};

    bool loadFromFolder(const QUrl &pathUrl, PlaylistItem *playlist) {
        auto url = !pathUrl.isEmpty() ? pathUrl : QUrl::fromUserInput(playlist->link);
        if (!url.isValid()) return false;
        qDebug() << "1" << pathUrl.toLocalFile() << pathUrl << pathUrl.path() << pathUrl.toDisplayString();
        QFileInfo pathInfo = QFileInfo(pathUrl.toLocalFile());
        qDebug() << "2" << pathInfo.absoluteFilePath() << pathInfo.absolutePath() << pathInfo.canonicalFilePath();
        if (!pathInfo.exists()) {
            oLog() << "Playlist" << pathInfo.absoluteFilePath() << "doesn't exist";
            return false;
        }
        QDir playlistDir = pathInfo.isDir() ? QDir(url.toLocalFile()) : pathInfo.dir();
        QStringList playableFiles = playlistDir.entryList(m_playableExtensions, QDir::Files);

        // Delete the playlist if no playable files
        if (playableFiles.isEmpty()) {
            oLog() << "Playlist" << "No files to play in" << playlistDir.absolutePath();
            return false;
        }

        playlist->name = playlistDir.dirName();
        playlist->displayName = playlistDir.dirName();
        playlist->link = playlistDir.absolutePath();
        playlist->m_historyFile = std::make_unique<QFile>(playlistDir.filePath(".mpv.history"));
        playlist->setIsLocalDir(true);
        playlist->clear();

        QString fileToPlay = "";
        int timestamp = 0;

        // If pathUrl is a directory, attempt to read history file for last played file
        if (pathInfo.isDir()) {
            if (playlist->m_historyFile->exists()) {
                bool fileOpened = playlist->m_historyFile->isOpen() ? true : playlist->m_historyFile->open(QIODevice::ReadOnly | QIODevice::Text);
                if (fileOpened) {
                    auto fileData = QTextStream(playlist->m_historyFile.get()).readAll().trimmed().split(":");
                    playlist->m_historyFile->close();
                    if (!fileData.isEmpty()) {
                        fileToPlay = fileData.first();
                        if (fileData.size() == 2) {
                            timestamp = fileData.last().toInt();
                        }
                    }
                } else {
                    rLog() << "Playlist" << "Failed to open history file";
                }
            }
        } else if (playableFiles.contains(pathInfo.fileName())){
            // Update the histroy if pathUrl is a filepath
            bool fileOpened = playlist->m_historyFile->isOpen() ? true : playlist->m_historyFile->open(QIODevice::WriteOnly | QIODevice::Text);
            if (fileOpened) {
                playlist->m_historyFile->write(pathInfo.fileName().toUtf8());
                playlist->m_historyFile->close();
                fileToPlay = pathInfo.fileName();
            }
            else {
                rLog() << "Playlist" << "Failed to open and update history file";
            }
        }

        // Keep track of the pointer to the last played file
        PlaylistItem *currentItemPtr = nullptr;

        static QRegularExpression fileNameRegex{ R"((?:[Ss](?<S>\d{1,2})[Ee](?<E>\d{1,3})[\s\-\.]*| (?<episode>\d{2,3}) ?[\s\-]*)(?<title>[^\(\)]+\w)?.*?\.\w{3,4}$)" };

        for (int i = 0; i < playableFiles.count(); i++) {
            auto file = playableFiles[i];
            QRegularExpressionMatch match = fileNameRegex.match(file);
            QString title;
            int season = 0;
            float episodeNumber;
            if (match.hasMatch()) {
                title = match.hasCaptured("title") ? match.captured("title").trimmed() : "";
                season = match.hasCaptured("S") ? match.captured("S").trimmed().toInt() : 0;
                auto episodeStr = match.hasCaptured("E") ? match.captured("E").trimmed() : (match.hasCaptured("episode") ? match.captured("episode") : "");
                bool ok;
                float ep = episodeStr.toFloat(&ok);
                episodeNumber = ok ? ep : i;
            }
            playlist->emplaceBack(season, episodeNumber,  playlistDir.absoluteFilePath(file), title, true);
            if (file == fileToPlay) {
                // Set current item
                currentItemPtr = playlist->m_children->last();
            }
        }


        // Sort the episodes in order
        std::stable_sort(playlist->m_children->begin(), playlist->m_children->end(),
                         [](const PlaylistItem *a, const PlaylistItem *b) {
                             return a->number < b->number;
                         });

        if (currentItemPtr) {
            playlist->setCurrentIndex(playlist->indexOf(currentItemPtr));
            if (timestamp != 0) {
                currentItemPtr->setTimestamp(timestamp);
            }
        }

        // if (playlist->getCurrentIndex() < 0)
        //     playlist->setCurrentIndex(0);

        return true;
    }

    Q_SLOT void onLocalDirectoryChanged(const QString &path);
    Q_SLOT void onLoadFinished();
private:
    PlayInfo m_currentPlayItem;
};

