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
    Q_PROPERTY(int listType READ getlistType WRITE setListType NOTIFY layoutChanged);
    enum{
        WATCHING,
        PLANNED,
        ON_HOLD,
        DROPPED,
    };
    int m_currentShowListIndex = -1;
    nlohmann::json m_jsonList;
    QVector<ShowResponse> m_list;
    QVector<int> m_watchingList;
    QVector<int> m_plannedList;
    QVector<int> m_onHoldList;
    QVector<int> m_droppedList;
    QVector<int>* m_currentList = &m_watchingList;

    QMap<int,QVector<int>*>
        listMap{
                {WATCHING, &m_watchingList},
                {PLANNED, &m_plannedList},
                {ON_HOLD, &m_onHoldList},
                {DROPPED, &m_droppedList},
                };

    int m_listType = WATCHING;


    int getlistType(){
        return m_listType;
    }
    void setListType(int listType){
        m_listType = listType;
        m_currentList = listMap[listType];
        emit layoutChanged ();
    }
public:
    WatchListModel(){
        std::ifstream infile(".watchlist");
        if (!infile.good()) { // file doesn't exist or is corrupted
            std::ofstream outfile(".watchlist"); // create new file
            outfile << "[]"; // write [] to file
            outfile.close();
        }else if(infile.peek() == '['){
            infile>> m_jsonList;
        }else{
            m_jsonList = nlohmann::json::array ();
        }

        for (int i = 0; i < m_jsonList.size (); ++i) {
            nlohmann::json& showItem = m_jsonList[i];
            ShowResponse show;
            show.link = QString::fromStdString(showItem["link"].get<std::string>());
            show.title = QString::fromStdString(showItem["title"].get<std::string>());
            show.coverUrl = QString::fromStdString(showItem["cover"].get<std::string>());
            show.provider = showItem["provider"].get<int>();
            show.setLastWatchedIndex(showItem["lastWatchedIndex"].get<int>());
            show.isInWatchList = true;
            show.listType = showItem["listType"].get<int>();
            switch(show.listType){
            case WATCHING:
                m_watchingList.push_back (i);
                break;
            case PLANNED:
                m_plannedList.push_back (i);
                break;
            case ON_HOLD:
                m_onHoldList.push_back (i);
                break;
            case DROPPED:
                m_droppedList.push_back (i);
                break;
            default:
                qWarning()<<"Invalid list type.";
                show.listType = WATCHING;
                showItem["listType"] = WATCHING;
                m_watchingList.push_back (i);
            }
            m_list.push_back(show);
        }
    };

    ~WatchListModel() {}

    Q_INVOKABLE void add(ShowResponse& show, int listType = WATCHING){
        if(show.isInWatchList || checkInList (show))return;

        if(show.object){
            show.object->setIsInWatchList (true);
            show.object->setListType (listType);
        }else{
            show.isInWatchList=true;
            show.listType=listType;
        }

        m_list.push_back (show);
        emit layoutChanged ();
        nlohmann::json showObj;
        showObj["title"]= show.title.toStdString ();
        showObj["cover"]= show.coverUrl.toStdString ();
        showObj["link"] = show.link.toStdString ();
        showObj["provider"] = show.provider;
        showObj["listType"] = listType;
        showObj["lastWatchedIndex"] = show.getLastWatchedIndex();
        m_jsonList.push_back (showObj);
        save();
    }

    Q_INVOKABLE void addCurrentShow(int listType = WATCHING){
        if(Global::instance ().currentShowObject ()->isInWatchList ()){
            auto link = Global::instance ().currentShowObject ()->link ().toStdString ();
            if(Global::instance ().currentShowObject ()->listType ()==listType)return;
            m_list[m_currentShowListIndex].listType= listType;
            emit layoutChanged();
            m_jsonList[m_currentShowListIndex] = listType;
            save();
            Global::instance ().currentShowObject ()->setListType (listType);
            return;
        }
        add(*Global::instance ().currentShowObject ()->getShow (), listType);
    }




    bool checkInList(const ShowResponse& show){
        std::string title = show.title.toStdString ();
        std::string coverUrl = show.coverUrl.toStdString ();
        std::string link = show.link.toStdString ();
        for(const auto& item:m_jsonList.items ()){
            nlohmann::json showItem = item.value();
            if(showItem["link"]==link){
                return true;
            }
        }
        return false;
    }

    void save(){
        std::ofstream output_file(".watchlist");
        output_file << m_jsonList.dump (4);
        output_file.close();
    }

    Q_INVOKABLE void remove(ShowResponse& show){
        show.isInWatchList = false;
        std::string link = show.link.toStdString ();
        for (size_t i = 0; i < m_jsonList.size(); i++) {
            if (m_jsonList[i]["link"] == link) {
                removeAtIndex (i);
                break;
            }
        }
        save();
    }

    Q_INVOKABLE void removeAtIndex(int index){
        if (index >= 0 && index < m_jsonList.size()) {
            m_jsonList.erase(m_jsonList.begin() + index);
            m_list.remove(index);
            emit layoutChanged ();
        }
    }

    Q_INVOKABLE void removeCurrentShow(){
        ShowResponse show = *Global::instance ().currentShowObject ()->getShow ();
        remove(show);
        Global::instance ().currentShowObject ()->setIsInWatchList(false);
        Global::instance ().currentShowObject ()->setListType(-1);
        //        qDebug()<<"IN LIST REMOVED: " <<Global::instance ().currentShowObject ()->getShow ()->getIsInWatchList ();
    }

    void updateCurrentShow(){
        update(*Global::instance ().currentShowObject ()->getShow ());
    }

    QString lastlink;

    Q_INVOKABLE void loadDetails(int index){
        int i;
        switch(m_listType){
        case WATCHING:
            i = m_watchingList[index];
            break;
        case PLANNED:
            i = m_plannedList[index];
            break;
        case ON_HOLD:
            i = m_onHoldList[index];
            break;
        case DROPPED:
            i = m_droppedList[index];
            break;
        default:
            return;
        }
        emit detailsRequested(m_list.at (i));
    }

    Q_INVOKABLE void move(int from, int to){
        //        if (from < 0 || from >= m_jsonList.size() || to < 0 || to >= m_jsonList.size()) return;
        QVector<int>* currentList;
        if(m_listType == WATCHING)currentList = &m_watchingList;
        else if(m_listType == PLANNED) currentList = &m_plannedList;
        else return;


        int actualFrom = currentList->at (from);
        int actualTo = currentList->at (to);

        qDebug()<<"currentList ="<<m_listType << "from"<<from<<"to"<<to;
        qDebug()<<m_list[actualFrom].title << "to" << m_list[actualTo].title;


        qDebug()<<"actualFrom"<<actualFrom<<"actualTo"<<actualTo;
        qDebug()<<"\n";
        m_list.move(actualFrom, actualTo);



        emit indexMoved (actualFrom, actualTo);

        auto element_to_move = m_jsonList[actualFrom];
        m_jsonList.erase(m_jsonList.begin() + actualFrom);
        m_jsonList.insert(m_jsonList.begin() + actualTo, element_to_move);
        save();
    }

    Q_INVOKABLE void moveEnded(){
        m_watchingList.clear ();
        m_plannedList.clear ();
        for(int i =0 ; i<m_list.count ();i++){
            int listType  = m_list[i].listType;
            if (listType == WATCHING) {
                m_watchingList.push_back (i);
            }else if (listType == PLANNED) {
                m_plannedList.push_back (i);
            }
        }
        emit layoutChanged ();
    }

    void update(const ShowResponse& show){
        std::string link = show.link.toStdString ();
        for (size_t i = 0; i < m_jsonList.size(); i++) {
            if (m_jsonList[i]["link"] == link) {
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
        auto link = Global::instance().currentShowObject ()->link ();
        for(int i = 0;i<m_list.count ();i++){
            const auto& item = m_list[i];
            if(item.link==link){
                Global::instance().currentShowObject()->setIsInWatchList(true);
                Global::instance().currentShowObject()->setLastWatchedIndex(item.lastWatchedIndex);
                Global::instance().currentShowObject()->setListType(item.listType);
                m_currentShowListIndex = -1;
                return true;
            }
        }
        Global::instance().currentShowObject()->setIsInWatchList(false);
        Global::instance().currentShowObject()->setListType(-1);
        m_currentShowListIndex = -1;
        //        Global::instance().currentShowObject()->setLastWatchedIndex(-1);
        return false;
    }

    void updateLastWatchedIndex(int index,int lastWatchedIndex){
        m_list[index].setLastWatchedIndex (lastWatchedIndex);
        m_jsonList[index]["lastWatchedIndex"]=lastWatchedIndex;
        save();
    }

private:
    enum{
        TitleRole = Qt::UserRole,
        CoverRole
    };
    int rowCount(const QModelIndex &parent) const;

    QVariant data(const QModelIndex &index, int role) const;

    QHash<int, QByteArray> roleNames() const;
};

#endif // WATCHLISTMODEL_H
