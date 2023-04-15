#ifndef CONSUMET9ANIME_H
#define CONSUMET9ANIME_H

#include "functions.hpp"
#include <parsers/showparser.h>
#include <Functions.hpp>



class Consumet9anime final:public ShowParser
{
    std::string m_lastSearch;
    bool m_widgetSearched = false;
    int providerEnum() override{
        return Providers::e_Consumet9anime;
    }
public:
    Consumet9anime();
    QString name() override {return "9anime";};
    std::string hostUrl()override{return "lol.com";};

    QVector<ShowResponse> search(QString query, int page, int type=0)override{
        m_lastSearch = query.toStdString ();
        std::string url = "https://api.consumet.org/anime/9anime/"+Functions::urlEncode (m_lastSearch)+"?page="+std::to_string (page);
        nlohmann::json response = client.get(url).json();
        m_canFetchMore = response["hasNextPage"].get <bool>();
        QVector<ShowResponse> animes;
        for(const auto& result : response["results"]){
            ShowResponse anime;
            anime.title = QS(result["title"].get<std::string> ());
            anime.link = QS(result["id"].get<std::string> ());
            anime.coverUrl = QS(result["image"].get<std::string> ());
            anime.provider = Providers::e_Consumet9anime;
            animes.push_back (anime);
        }
        return animes;
    };

    QVector<ShowResponse> popular(int page,int type)override{
        Q_UNUSED(type);
        return widgetSearch("trending",page);
    };

    QVector<ShowResponse> latest(int page,int type)override{
        Q_UNUSED(type);
        return widgetSearch ("updated-sub",page);
    };

    QVector<ShowResponse> widgetSearch(std::string path,int page)
    {
        m_widgetSearched = true;
        m_currentPage = page;
        m_lastSearch = path;
        std::string url = "https://9anime.id/ajax/home/widget/" + path +"?page=" + std::to_string(page);
        QVector<ShowResponse> animes = parseAnimes (url);
        m_canFetchMore = !animes.isEmpty ();
        return animes;
    };

    QVector<ShowResponse> fetchMore()override{
        if(m_widgetSearched){
            return widgetSearch (m_lastSearch,++m_currentPage);
        }else{
            return search (QS(m_lastSearch),++m_currentPage);
        }
    };

    ShowResponse loadDetails(ShowResponse anime)override{
        auto url = "https://api.consumet.org/anime/9anime/info/"+anime.link.toStdString ();
        auto response = client.get (url).json ();
        //        qDebug()<<anime->link;

        if(!response["description"].is_null ()){
            anime.description = QS(response["description"].get<std::string> ());
        }
        if(!response["releaseDate"].is_null ()){
            anime.releaseDate = QS(response["releaseDate"].get<std::string> ());
        }
        if(!response["views"].is_null ()){
            anime.views = response["views"].get<int> ();
        }
        //        anime->rating = QS(response["score"].get<float> ());
        for(const auto& item: response["episodes"].items()){
            nlohmann::json ep = item.value();
            Episode episode;
            episode.number = ep["number"].get<int> ();
            if(!ep["title"].is_null ()){
                episode.title = QS(ep["title"].get<std::string> ());
            }
            episode.isFiller=ep["isFiller"].get<bool> ();
            episode.link = ep["id"].get<std::string> ();
            anime.episodes.push_back (episode);
        }
        return anime;
    };

    QVector<VideoServer> loadServers(Episode *episode)override{
        QVector<VideoServer> servers{
            VideoServer{"vidcloud",episode->link},
            VideoServer{"streamsb",episode->link},
            VideoServer{"vidstreaming",episode->link},
            VideoServer{"streamtape",episode->link},
            VideoServer{"vidcloud",episode->link}
        };
        return servers;
    };

    void extractSource(VideoServer *server)override{

        qDebug()<<"https://api.consumet.org/anime/9anime/watch/"+server->link+"?server="+server->name.toStdString ();
        auto response = client.get ("https://api.consumet.org/anime/9anime/watch/"+server->link+"&server="+server->name.toStdString ()).json ();
        qDebug()<<response.dump ();
        server->source = QS(response["sources"][0]["url"].get<std::string> ());
    };
private:
    QVector<ShowResponse> parseAnimes(std::string url){
        CSoup document(client.get(url).json ()["result"]);
        QVector<ShowResponse> animes;
        pugi::xpath_node_set items = document.select("//div[@class='item']");
        items.forEach ([&](pugi::xpath_node node){
            ShowResponse anime;
            pugi::xpath_node anchor = node.selectFirst (".//a[@class='name d-title']");
            std::string title = anchor.node ().child_value ();
            Functions::replaceAll (title,"\n"," ");
            anime.title= title.c_str ();
            anime.coverUrl = node.selectFirst (".//img").attr ("src").as_string ();
            anime.provider = Providers::e_Consumet9anime;
            anime.link = Functions::findBetween (anchor.attr ("href").as_string (),"/watch/","/ep-").c_str ();
            anime.latestTxt = node.selectText(".//span[@class='ep-status total']/span");
            animes.push_back (anime);
        });
        return animes;
    };

    Status parseStatus(std::string statusString){
        if (statusString=="Releasing") {
            return Status::Ongoing;}
        else if(statusString=="Completed"){
            return Status::Completed;
        }
        return Status::Completed;
    }
};

#endif // CONSUMET9ANIME_H
