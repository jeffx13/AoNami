#ifndef GOGOANIME_H
#define GOGOANIME_H


#include "parsers/showparser.h"
#include <network/client.h>
#include <QDebug>
#include <parsers/extractors/gogocdn.h>
#include <functions.hpp>

class Gogoanime : public ShowParser
{
    Q_OBJECT
    QString m_lastSearch;
    enum e_lastSearch{
        LATEST,
        POPULAR
    };
    int providerEnum() override{
        return Providers::e_Gogoanime;
    }
public:
    Gogoanime();
    QString name() override{
        return "Gogoanime";
    }
    std::string hostUrl() override{
        return  "https://gogoanime.gr";
    }

    QVector<ShowResponse> search(QString query, int page, int type=0) override{
        m_currentPage=page;
        m_lastSearch=query;
        std::string url = "https://gogoanime.gr/search.html?keyword=" + Functions::urlEncode(query.toStdString ()) + "&page="+std::to_string(page);
        QVector<ShowResponse> animes;
        client.get(url).document().select("//ul[@class='items']/li").forEach ([&](pugi::xpath_node node) {
            ShowResponse anime;
            auto anchor = node.selectFirst(".//p[@class=\"name\"]/a");
            anime.title = anchor.attr("title").as_string();
            anime.coverUrl =
                node.selectFirst(".//img").attr("src").as_string();
            anime.link = anchor.attr ("href").as_string ();
            anime.provider = Providers::e_Gogoanime;
            anime.releaseDate = node.selectText (".//p[@class=\"released\"]"); //TODO remove released
            anime.releaseDate.remove (0,10);
            animes.append (anime);
        });
        m_canFetchMore=!animes.empty ();
        return animes;
    }

    QVector<ShowResponse> popular(int page, int type=0) override{
        m_currentPage=page;
        QVector<ShowResponse> animes;
        client.get ("https://ajax.gogo-load.com/ajax/page-recent-release-ongoing.html?page="+std::to_string (page)).document ()
            .select ("//div[@class='added_series_body popular']/ul/li").forEach([&](pugi::xpath_node element){
                ShowResponse anime;
                pugi::xpath_node anchor = element.selectFirst ("a");
                anime.link = QS(hostUrl()+anchor.attr("href").as_string ());
                anime.coverUrl = QS(Functions::findBetween (anchor.selectFirst(".//div[@class='thumbnail-popular']").attr ("style").as_string (),"url('","');"));
                anime.provider = Providers::e_Gogoanime;
                anime.latestTxt=QS(element.selectText (".//p[last()]/a"));
                anime.title = QS(anchor.attr ("title").as_string ());
                animes.push_back (anime);
            });
        m_canFetchMore=!animes.empty ();
        m_lastSearch = "popular";
        return animes;
    }

    QVector<ShowResponse> latest(int page, int type=0) override{
        m_currentPage=page;
        QVector<ShowResponse> animes;
        client.get("https://ajax.gogo-load.com/ajax/page-recent-release.html?page=" + std::to_string(page) + "&type=1").document()
            .select("//ul[@class='items']/li").forEach([&](const pugi::xpath_node& element) {
                ShowResponse anime;
                auto anchor = element.selectFirst (".//p[@class='name']/a");
                std::string title = anchor.node ().child_value ();
                Functions::replaceAll (title,"\n"," ");
                anime.title=title.c_str ();
                anime.link = QS("https://gogoanime.gr/category" + Functions::substringBefore (anchor.attr ("href").as_string (),"-episode"));
                anime.coverUrl = element.selectFirst(".//div/a/img").attr("src").as_string();
                anime.latestTxt = element.selectText(".//p[@class='episode']");
                anime.provider = Providers::e_Gogoanime;
                animes.push_back(anime);
            });
        m_lastSearch = "latest";
        m_canFetchMore=!animes.empty ();
        return animes;
    }

    void loadDetail(ShowResponse *anime) override{
        auto url = anime->link;
        auto doc = client.get(url.toStdString ()).document ();
        anime->description = doc.selectFirst ("//span[contains(text(),'Plot Summary')]/parent::*/text()").toString ().substr(1).c_str ();
        anime->status=doc.selectFirst ("//span[contains(text(),'Status')]/following-sibling::*/text()").toString ().c_str ();
        anime->description.replace ("\n"," ");
        doc.select ("//span[contains(text(),\"Genre\")]/following-sibling::*/text()").forEach ([&](pugi::xpath_node node){
            QString genre = node.toString ().c_str ();
            genre.replace ("\n"," ");
            anime->genres.push_back (genre);
        });
        std::string lastEpisode = doc.selectFirst ("//ul[@id=\"episode_page\"]/li[last()]/a").attr ("ep_end").value ();
        std::string animeId = doc.selectFirst ("//input[@id=\"movie_id\"]").attr ("value").value ();
        auto epList = client.get("https://ajax.gogo-load.com/ajax/load-list-episode?ep_start=0&ep_end="+lastEpisode+"&id="+animeId).document ()
                          .select("//ul/li/a").map<Episode>([&](pugi::xpath_node node) {
                              Episode ep;
                              ep.number = std::stoi (node.selectText ("div[@class=\"name\"]"));
                              ep.link = node.attr ("href").value ();
                              return ep;
                          });
        std::reverse(epList.begin(), epList.end());
        anime->episodes = QVector<Episode>(epList.begin (),epList.end ());
        emit episodesFetched (anime->episodes);
    };

    QVector<VideoServer> loadServers(Episode *episode) override{

        auto url = hostUrl ()+episode->link;

        //        qDebug()<<url<<client.get(url).document ().selectText ("//div[@class='anime_muti_link']/ul/li/a");

        auto servers = client.get(url).document ().select("//div[@class='anime_muti_link']/ul/li").map <VideoServer>([&](pugi::xpath_node node){
            VideoServer server;
            server.name = node.selectText (".//a");
            std::string link = node.selectFirst (".//a"). attr("data-video").value ();
            Functions::httpsIfy(link);
            server.link = link;
            server.headers["referer"] = QS(hostUrl ());
            //            qDebug()<<server.name;
            //            qDebug()<<server.link;
            return server;
        });

        return QVector<VideoServer>(servers.begin (),servers.end ());
    };

    void extractSource(VideoServer *server) override{
        auto domain = server->link;
        VideoExtractor *extractor;
        //rank the servers by preference
        //        QHash<int,QHash<QString,QString>> serverPreference;
        if (Functions::containsSubstring(domain, "gogo")
            || Functions::containsSubstring(domain, "goload")
            || Functions::containsSubstring(domain, "playgo")
            || Functions::containsSubstring(domain, "anihdplay")) {
            extractor = new GogoCDN;
            extractor->extract(server);
            delete extractor;
        } else if (Functions::containsSubstring(domain, "sb")
                   || Functions::containsSubstring(domain, "sss")) {
            //            extractor = new StreamSB(server);
        } else if (Functions::containsSubstring(domain, "fplayer")
                   || Functions::containsSubstring(domain, "fembed")) {
            //            extractor = new FPlayer(server);
        }

    };

    QVector<ShowResponse> fetchMore() override {
        if(m_lastSearch[0]=='l'){
            return latest (++m_currentPage,true);
        }else if(m_lastSearch[0]=='p'){
            return popular (++m_currentPage,true);
        }else{
            return search (m_lastSearch,++m_currentPage);
        }
    }
};

#endif // GOGOANIME_H
