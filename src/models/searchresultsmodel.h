#ifndef SEARCHRESULTSMODEL_H
#define SEARCHRESULTSMODEL_H

#include <QAbstractListModel>
#include <parsers/showresponse.h>
#include <parsers/showparser.h>
#include <global.h>
#include <QtConcurrent>
#include <WatchListManager.h>
#include <parsers/providers/gogoanime.h>
#include <parsers/providers/nivod.h>


class SearchResultsModel : public QAbstractListModel
{
    Q_OBJECT
    bool fetchingMore = false;
    QTimer timeoutTimer;
    const int timeoutDuration = 500;
public:
    explicit SearchResultsModel(QObject *parent = nullptr)
        : QAbstractListModel(parent){
        timeoutTimer.setSingleShot(true);

        //        QObject::connect(&m_searchWatcher, &QFutureWatcher<QVector<ShowResponse>>::canceled, [&]() {
        //            qDebug() << "Took too long to load shows.";
        //            emit loadingEnd();
        //        });
        //        QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
        //            timeoutTimer.stop();

        //        });
        //        QObject::connect(&m_searchWatcher, &QFutureWatcher<int>::started, [&](){
        //            timeoutTimer.start(timeoutDuration);
        //        });
        QObject::connect(&m_searchWatcher, &QFutureWatcher<QVector<ShowResponse>>::finished,this, [&]() {
            try{
                QVector<ShowResponse> results = m_searchWatcher.future ().result ();
                if(fetchingMore){
                    mList+=results;
                }else{
                    mList=results;
                }
                emit layoutChanged ();
                emit postItemsAppended();
                emit loadingEnd();
                fetching=false;
            }catch(const QException& e){
                qDebug()<<e.what ();
                emit loadingEnd();
                fetching=false;
            }
        });


        //        QObject::connect(&m_detailLoadingWatcher, &QFutureWatcher<int>::canceled, [&]() {
        //            qDebug() << "Took too long to load details.";
        //            emit loadingEnd();
        //        });
        QObject::connect(&m_detailLoadingWatcher, &QFutureWatcher<void>::finished,this, [&]() {
//            Global::instance().currentShowObject ()->emitPropertyChanged();
//            emit detailsLoaded();
            emit loadingEnd ();
        });
    }

    Q_INVOKABLE void search(QString query,int page,int type){
        fetching=true;
        fetchingMore = false;
        emit loadingStart();
        timeoutTimer.start(timeoutDuration);
        m_searchWatcher.setFuture (QtConcurrent::run(&ShowParser::search, Global::instance ().getCurrentSearchProvider (), query, page, type));
    }

    Q_INVOKABLE void latest(int page,int type){
        fetching=true;
        fetchingMore = false;
        emit loadingStart();

        m_searchWatcher.setFuture (QtConcurrent::run (&ShowParser::latest,Global::instance ().getCurrentSearchProvider (),page,type));
    }

    Q_INVOKABLE void popular(int page,int type){
        fetching=true;
        fetchingMore = false;
        emit loadingStart();
        m_searchWatcher.setFuture (QtConcurrent::run (&ShowParser::popular,Global::instance ().getCurrentSearchProvider (),page,type));
    }

    Q_INVOKABLE void loadDetails(int index){
        getDetails(mList[index]);
    }

public:
    QVector<ShowResponse> list() const{return mList;};
    Q_INVOKABLE ShowResponse* get(int index) {return &mList[index];}
    Q_INVOKABLE ShowResponse itemAt(int index) const {return mList[index];}

public slots:
    void getDetails(ShowResponse show){
        emit loadingStart();
        if(Global::instance().currentShowObject ()->getShow()->title == show.title){
            Global::instance().currentShowObject ()->setShow (*Global::instance().currentShowObject ()->getShow());
            emit loadingEnd ();
            return;
        }

        m_detailLoadingWatcher.setFuture (QtConcurrent::run ([&](){
            Global::instance().getProvider(show.provider)->loadDetail (&show);
            Global::instance().currentShowObject ()->setShow(show);
        }));
    }

private:
    bool fetching=false;
    QFutureWatcher<QVector<ShowResponse>> m_searchWatcher{};
    QFutureWatcher<void> m_detailLoadingWatcher{};
    enum{
        TitleRole = Qt::UserRole,
        CoverRole
    };
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override{
        QHash<int, QByteArray> names;
        names[TitleRole] = "title";
        names[CoverRole] = "cover";
        return names;
    };
    QVector<ShowResponse> mList;
public:
    Q_INVOKABLE void fetchMore(){
        fetching=true;
        fetchingMore = true;
        emit loadingStart();
        m_searchWatcher.setFuture (QtConcurrent::run (&ShowParser::fetchMore,Global::instance ().getCurrentSearchProvider ()));
    };
    Q_INVOKABLE bool canFetchMore()const{
        if(fetching)return false;
        return Global::instance ().getCurrentSearchProvider ()->canFetchMore ();
    }
signals:
    void currentShowChanged(void);
    void fetchMoreResults(void);
    void loadingStart(void);
    void loadingEnd(void);
    void detailsLoaded(void);
    void sourceFetched(QString link);
    void postItemsAppended(void);
};



#endif // SEARCHRESULTSMODEL_H
