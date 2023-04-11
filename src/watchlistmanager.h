#ifndef WATCHLISTMANAGER_H
#define WATCHLISTMANAGER_H

#include <global.h>
#include <fstream>
#include <iostream>
#include <parsers/showresponse.h>
#include <nlohmann/json.hpp>


class WatchListManager: public QObject
{
    Q_OBJECT
    enum{
        WATCHING,
        PLANNED,
    };
    nlohmann::json watchList;
    QVector<ShowResponse> m_list;
public:
    Q_INVOKABLE void load(){
        std::ifstream infile(".watchlist");
        if (!infile.good()) { // file doesn't exist or is corrupted
            std::ofstream outfile(".watchlist"); // create new file
            outfile << "[]"; // write [] to file
            outfile.close();
        }else if(infile.peek() == '['){
            infile>> watchList;
        }else{
            watchList = nlohmann::json::array ();
        }

        emit loaded(QString::fromStdString (watchList.dump ()));
        for(const auto& item:watchList.items ()){
            nlohmann::json showItem = item.value();
            ShowResponse show;
            show.link = QString::fromStdString (showItem["link"].get <std::string>());
            show.title = QString::fromStdString (showItem["title"].get <std::string>());
            show.coverUrl = QString::fromStdString (showItem["cover"].get <std::string>());
            show.provider = showItem["provider"].get <int>();
            show.setLastWatchedIndex (showItem["lastWatchedIndex"].get <int>());
            show.setIsInWatchList (true);
            m_list.push_back (show);
        }
    }

    Q_INVOKABLE void add(ShowResponse& show, int listType = WATCHING){
        if(show.getIsInWatchList () || checkInList (show))return;
        show.setIsInWatchList(true);
        nlohmann::json showObj;
        showObj["title"]= show.title.toStdString ();
        showObj["cover"]= show.coverUrl.toStdString ();
        showObj["link"] = show.link.toStdString ();
        showObj["provider"] = show.provider;
        showObj["listType"] = listType;
        showObj["lastWatchedIndex"] = show.getLastWatchedIndex();
        switch (listType) {
        case WATCHING:
            emit watchingAdded (QString::fromStdString (showObj.dump ()));
            break;
        case PLANNED:
            emit plannedAdded(QString::fromStdString (showObj.dump ()));
            break;
        }
        watchList.push_back (showObj);
        m_list.push_back (show);
        save();
    }

    Q_INVOKABLE void addCurrentShow(int listType = WATCHING){
        ShowResponse show = *Global::instance ().currentShowObject ()->getShow ();
        add(show, listType);
        Global::instance ().currentShowObject ()->setIsInWatchList(true);
//        qDebug()<<"IN LIST ADDED: " << Global::instance ().currentShowObject ()->getShow ()->getIsInWatchList ();

    }

    bool checkInList(const ShowResponse& show){
        std::string title = show.title.toStdString ();
        std::string coverUrl = show.coverUrl.toStdString ();
        std::string link = show.link.toStdString ();
        for(const auto& item:watchList.items ()){
            nlohmann::json showItem = item.value();
            if(showItem["link"]==link){
                return true;
            }
        }
        return false;
    }

    void save(){
        std::ofstream output_file(".watchlist");
        output_file << watchList.dump ();
        output_file.close();
    }

    Q_INVOKABLE void remove(ShowResponse& show){
        show.setIsInWatchList(false);
        std::string link = show.link.toStdString ();
        for (size_t i = 0; i < watchList.size(); i++) {
            if (watchList[i]["link"] == link) {
                removeAtIndex (i);
                break;
            }
        }
        save();
    }

    Q_INVOKABLE void removeAtIndex(int index){
        if (index >= 0 && index < watchList.size()) {
            watchList.erase(watchList.begin() + index);
            m_list.remove(index);
            emit removedAtIndex (index);
        }
    }

    Q_INVOKABLE void removeCurrentShow(){
        ShowResponse show = *Global::instance ().currentShowObject ()->getShow ();
        remove(show);
        Global::instance ().currentShowObject ()->setIsInWatchList(false);
//        qDebug()<<"IN LIST REMOVED: " <<Global::instance ().currentShowObject ()->getShow ()->getIsInWatchList ();
    }

    void update(const ShowResponse& show){
        std::string link = show.link.toStdString ();
        for (size_t i = 0; i < watchList.size(); i++) {
            if (watchList[i]["link"] == link) {
                watchList[i]["lastWatchedIndex"] = show.getLastWatchedIndex ();
                m_list[i].setLastWatchedIndex (show.getLastWatchedIndex ());
                break;
            }
        }
        save();
    }

    void updateCurrentShow(){
        update(*Global::instance ().currentShowObject ()->getShow ());
    }

    static WatchListManager& instance()
    {
        static WatchListManager s_instance;
        return s_instance;
    }

    Q_INVOKABLE void loadDetails(int index){
        auto show = m_list.at (index);
        emit detailsRequested(show);
    }
    Q_INVOKABLE void move(int index1, int index2){
        std::swap(watchList[index1], watchList[index2]);
        std::swap(m_list[index1], m_list[index2]);
        save();
    }

signals:
    void loaded(QString showObject);
    void plannedAdded(QString showObject);
    void watchingAdded(QString showObject);
    void removedAtIndex(int index);
    void detailsRequested(const ShowResponse& show);
public slots:
    bool checkCurrentShowInList(){
        ShowResponse show = Global::instance().currentShow();
        for(const auto& item:m_list){
            if(item.link==show.link){
                Global::instance().currentShowObject()->setIsInWatchList(true);
                Global::instance().currentShowObject()->setLastWatchedIndex(item.getLastWatchedIndex ());
                return true;
            }
        }
        Global::instance().currentShowObject()->setIsInWatchList(false);
        Global::instance().currentShowObject()->setLastWatchedIndex(-1);
        return false;
    }



private:
    WatchListManager(){};
    ~WatchListManager() {} // Private destructor to prevent external deletion.
    WatchListManager(const WatchListManager&) = delete; // Disable copy constructor.
    WatchListManager& operator=(const WatchListManager&) = delete; // Disable copy assignment.
};

#endif // WATCHLISTMANAGER_H
