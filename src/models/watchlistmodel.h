#ifndef WATCHLISTMODEL_H
#define WATCHLISTMODEL_H

#include <global.h>
#include <fstream>
#include <iostream>
#include <QAbstractListModel>
#include <parsers/showresponse.h>
#include <nlohmann/json.hpp>


class WatchListModel: public QAbstractListModel
{
    Q_OBJECT
    enum{
        WATCHING,
        PLANNED,
    };
    nlohmann::json jsonList;
    QVector<ShowResponse> m_list;
public:
    WatchListModel(){
        std::ifstream infile(".watchlist");
        if (!infile.good()) { // file doesn't exist or is corrupted
            std::ofstream outfile(".watchlist"); // create new file
            outfile << "[]"; // write [] to file
            outfile.close();
        }else if(infile.peek() == '['){
            infile>> jsonList;
        }else{
            jsonList = nlohmann::json::array ();
        }

        for(const auto& item:jsonList.items ()){
            nlohmann::json showItem = item.value();
            ShowResponse show;
            show.link = QString::fromStdString (showItem["link"].get <std::string>());
            show.title = QString::fromStdString (showItem["title"].get <std::string>());
            show.coverUrl = QString::fromStdString (showItem["cover"].get <std::string>());
            show.provider = showItem["provider"].get <int>();
            show.setLastWatchedIndex (showItem["lastWatchedIndex"].get <int>());
            show.isInWatchList = true;
            m_list.push_back (show);
        }
    };
    ~WatchListModel() {}


    Q_INVOKABLE void add(ShowResponse& show, int listType = WATCHING){
        if(show.isInWatchList || checkInList (show))return;
        show.isInWatchList=true;
        show.listType=listType;
        m_list.push_back (show);
        emit layoutChanged ();




        nlohmann::json showObj;
        showObj["title"]= show.title.toStdString ();
        showObj["cover"]= show.coverUrl.toStdString ();
        showObj["link"] = show.link.toStdString ();
        showObj["provider"] = show.provider;
        showObj["listType"] = listType;
        showObj["lastWatchedIndex"] = show.getLastWatchedIndex();
        jsonList.push_back (showObj);
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
        for(const auto& item:jsonList.items ()){
            nlohmann::json showItem = item.value();
            if(showItem["link"]==link){
                return true;
            }
        }
        return false;
    }

    void save(){
        std::ofstream output_file(".watchlist");
        output_file << jsonList.dump ();
        output_file.close();
    }

    Q_INVOKABLE void remove(ShowResponse& show){
        show.isInWatchList = false;
        std::string link = show.link.toStdString ();
        for (size_t i = 0; i < jsonList.size(); i++) {
            if (jsonList[i]["link"] == link) {
                removeAtIndex (i);
                break;
            }
        }
        save();
    }

    Q_INVOKABLE void removeAtIndex(int index){
        if (index >= 0 && index < jsonList.size()) {
            jsonList.erase(jsonList.begin() + index);
            m_list.remove(index);
            emit layoutChanged ();
        }
    }

    Q_INVOKABLE void removeCurrentShow(){
        ShowResponse show = *Global::instance ().currentShowObject ()->getShow ();
        remove(show);
        Global::instance ().currentShowObject ()->setIsInWatchList(false);
        //        qDebug()<<"IN LIST REMOVED: " <<Global::instance ().currentShowObject ()->getShow ()->getIsInWatchList ();
    }


    void updateCurrentShow(){
        update(*Global::instance ().currentShowObject ()->getShow ());
    }

    QString lastlink;

    Q_INVOKABLE void loadDetails(int index){
        emit detailsRequested(m_list.at (index));
    }

    Q_INVOKABLE void move(int from, int to){
        if (from < 0 || from >= jsonList.size() || to < 0 || to >= jsonList.size()) {
            return;
        }
        m_list.move(from, to);
        emit indexMoved (from, to);
        auto element_to_move = jsonList[from];
        jsonList.erase(jsonList.begin() + from);
        jsonList.insert(jsonList.begin() + to, element_to_move);
        save();
    }

    Q_INVOKABLE void moveEnded(){
        emit layoutChanged ();
    }

    void update(const ShowResponse& show){
        std::string link = show.link.toStdString ();
        for (size_t i = 0; i < jsonList.size(); i++) {
            if (jsonList[i]["link"] == link) {
                updateLastWatchedIndex(i,show.getLastWatchedIndex ());
                break;
            }
        }
    }

    int getIndex(const QString& link){
        for (int i = 0; i < m_list.size(); i++) {
            if (m_list[i].link == link) {
                return i;
                break;
            }
        }
        return -1;
    }

signals:
    void detailsRequested(ShowResponse show);

    void indexMoved(int from,int to);

public slots:
    bool checkCurrentShowInList(){
        for(const auto& item:m_list){
            if(item.link==Global::instance().currentShowObject ()->link ()){
                Global::instance().currentShowObject()->setIsInWatchList(true);
                Global::instance().currentShowObject()->setLastWatchedIndex(item.getLastWatchedIndex ());
                return true;
            }
        }
        Global::instance().currentShowObject()->setIsInWatchList(false);
//        Global::instance().currentShowObject()->setLastWatchedIndex(-1);
        return false;
    }
    void updateLastWatchedIndex(int index,int lastWatchedIndex){
        m_list[index].setLastWatchedIndex (lastWatchedIndex);
        jsonList[index]["lastWatchedIndex"]=lastWatchedIndex;
        save();
    }
public:
    enum{
        TitleRole = Qt::UserRole,
        CoverRole
    };
    int rowCount(const QModelIndex &parent) const
    {
        if (parent.isValid())
            return 0;
        return m_list.count ();
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case TitleRole:
            return m_list[index.row()].title;
            break;
        case CoverRole:
            return m_list[index.row()].coverUrl;
            break;
        default:
            break;
        }
        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const{
        QHash<int, QByteArray> names;
        names[TitleRole] = "title";
        names[CoverRole] = "cover";
        return names;
    }
};

#endif // WATCHLISTMODEL_H
