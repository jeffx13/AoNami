#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H


#include "models/episodelistmodel.h"
#include "parsers/showparser.h"
#include <QDir>
#include <QAbstractListModel>
#include <global.h>
#include <WatchListManager.h>
#include <QtConcurrent>


class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    ShowParser *currentProvider;
    int playlistIndex = -1;
    QFile* m_historyFile;
    EpisodeListModel* episodeListModel;
    QFutureWatcher<QString> m_watcher{};
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(Episode currentItem READ currentItem NOTIFY currentIndexChanged)
    Q_PROPERTY(int size READ size CONSTANT)
public:
    int currentIndex()const {return playlistIndex;};
    Episode currentItem()const {return m_playlist[playlistIndex];};
    int size(){
        if(online){
            return m_playlist.size ();
        }else{
            return m_folderPlaylist.size ();
        }
    }
    explicit PlaylistModel(EpisodeListModel* episodeListModel,QObject *parent = nullptr):QAbstractListModel(parent){
        this->episodeListModel = episodeListModel;
        connect(&m_watcher, &QFutureWatcher<QString>::finished,this,[&]() {
            QString results = m_watcher.future ().result ();
            emit sourceFetched(results);
        });
    };
    ~PlaylistModel(){
        delete m_historyFile;
    }
    enum{
        TitleRole = Qt::UserRole,
        NumberRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadFolder(const QUrl& path){
        QDir directory(path.toLocalFile()); // replace with the path to your folder
        QStringList filters;
        filters << "*.mp4";
        QFileInfoList fileList = directory.entryInfoList(filters, QDir::Files);
        if(fileList.empty ())return;
        int loadIndex=0;
        std::sort(fileList.begin(), fileList.end(), [](const QFileInfo &a, const QFileInfo &b) {
            QString aName = a.fileName().split('.')[0]; // Get the part of the file name before the ".mp4" extension
            QString bName = b.fileName().split('.')[0];
            int aNumber = aName.split('_')[0].toInt(); // Get the number in the file name
            int bNumber = bName.split('_')[0].toInt();
            return aNumber < bNumber;
        });
        if(m_historyFile){
            delete m_historyFile;
            m_historyFile=nullptr;
        }
        QString lastWatched;
        m_historyFile = new QFile(directory.filePath(".mpv.history"));
        if (directory.exists(".mpv.history")) {
            if (m_historyFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
                lastWatched = QTextStream(m_historyFile).readAll().trimmed();
                //                qDebug()<<lastWatched;
                m_historyFile->close ();
            }
        }

        m_folderPlaylist.clear ();
        for (int i = 0; i < fileList.size(); i++) {
            const QFileInfo& fileInfo = fileList.at(i);
            QString filePath = fileInfo.absoluteFilePath();
            m_folderPlaylist << filePath;
            if(fileInfo.fileName()==lastWatched){
                loadIndex=i;
            }
        }

        online=false;
        loadSource (loadIndex);
    }

private:
    QVector<QString> m_folderPlaylist;
    QVector<Episode> m_playlist;
    bool online = true;
public:
    bool hasNextItem(){
        if(online)return playlistIndex<m_playlist.size ()-1;
        return playlistIndex<m_folderPlaylist.size ()-1;
    }
    bool hasPrecedingItem(){
        return playlistIndex>0;
    }
    void playNextItem(){
        loadOffset (1);
    }
    void playPrecedingItem(){
        loadOffset (-1);
    }
    Q_INVOKABLE void loadSource(int index){
        emit loadingStart();

        this->playlistIndex = index;
        emit currentIndexChanged();
        if(online){
            Global::instance().currentShowObject ()->setLastWatchedIndex (index);
            m_watcher.setFuture (QtConcurrent::run ([&](){
                auto servers=currentProvider->loadServers (&m_playlist[playlistIndex]); //todo make concurrent currentprovider bug
                currentProvider->extractSource (&servers.first ());

                if(Global::instance().currentShowObject ()->getShow ()->getIsInWatchList ()){
                    qDebug()<<"SET INDEX"<<Global::instance().currentShowObject ()->getShow ()->getLastWatchedIndex ();
                    WatchListManager::instance().updateCurrentShow();
                }
                return servers.first ().source;
//                emit sourceFetched(servers.first ().source);
            }));
        }else{
            emit sourceFetched(m_folderPlaylist[playlistIndex]);
            if (m_historyFile->open(QIODevice::WriteOnly)) {
                QTextStream stream(m_historyFile);
                stream<<m_folderPlaylist[index].split ("/").last ();
                m_historyFile->close ();
            }
        }

        emit loadingEnd();

    }

    Q_INVOKABLE void loadOffset(int offset){
        playlistIndex += offset;
        loadSource(playlistIndex);
    }
signals:
    void loadingStart(void);
    void loadingEnd(void);
    void sourceFetched(QString link);
    void currentIndexChanged(void);

private slots:
    void listChanged(){
        emit layoutChanged ();
    }

public slots:
    void syncList(ShowParser* currentProvider){
        this->currentProvider=currentProvider;
        online=true;
        m_playlist=episodeListModel->list ();
        emit layoutChanged ();
    }

};

#endif // PLAYLISTMODEL_H
