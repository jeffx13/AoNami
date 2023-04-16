#ifndef NINEANIME_H
#define NINEANIME_H


#include "nlohmann/json.hpp"
#include <functions.hpp>
#include <iostream>
#include <network/client.h>
#include <parsers/showparser.h>
#include <string>
#include <string>
#include <cmath>
#include <QtConcurrent>

class NineAnime : public ShowParser
{
    struct Keys{
        std::string encrypt;
        std::string decrypt;
        std::vector<std::string> pre;
        std::vector<std::string> post;
        std::vector<std::vector<std::string>> operations;
    };
    std::string m_lastSearch;
    bool m_widgetSearched = true;

public:
    void getKeys(){
        std::string decrypted = Functions::rc4 ("ANIMEJEFF",Functions::base64Decode (client.get("https://raw.githubusercontent.com/AnimeJeff/Overflow/main/syek").body));
        //        qDebug()<<QString::fromStdString (decrypted);
        while(decrypted.back ()!='}'){
            decrypted.pop_back ();
        }
        nlohmann::json keysJson =nlohmann::json::parse (decrypted);
        this->keys.encrypt=keysJson["encrypt"].get <std::string>();
        this->keys.decrypt=keysJson["decrypt"].get <std::string>();
        this->keys.pre=keysJson["pre"].get <std::vector<std::string>>();
        this->keys.post=keysJson["post"].get <std::vector<std::string>>();
        this->keys.operations=keysJson["operations"].get <std::vector<std::vector<std::string>>>();
        //        qDebug()<<QString::fromStdString (encodeVrf ("FDqWAs0=,HT-YDc0l",keys));
    }
    Keys keys;
public:
    NineAnime(){
        getKeys ();
    };
    ~NineAnime()=default;

public:

    static std::string encodeVrf(const std::string& str,const Keys& keys){
        std::string ciphered=Functions::rc4(keys.encrypt,Functions::urlEncode (str));
        std::string encoded = Functions::base64Encode (ciphered);
        std::string encrypted = encrypt (encoded,keys.pre,keys.operations,keys.post);
        std::string output = Functions::urlEncode (Functions::base64Encode (encrypted));
        return output;
    };
    static std::string decodeVrf(const std::string& str,const std::string& key="hlPeNwkncH0fq9so"){
        return Functions::urlDecode(Functions::rc4 (key,Functions::base64Decode(str)));
    };
    static std::string encrypt(const std::string& str,const std::vector<std::string>& pre, const std::vector<std::vector<std::string>>& operations,const std::vector<std::string>& post){
        std::string output = str;
        auto process = [&](const std::vector<std::string>& steps)
        {
            for(const std::string& step:steps){
                if(step == "rot13"){
                    output = Functions::rot13 (str);
                }else if(step == "reverse"){
                    std::reverse(output.begin(), output.end());
                }
            }
        };
        process(pre);
        std::string u = "";
        std::vector<std::string> operation;
        const int mod = operations.size ();
        for(unsigned int i=0;i<output.length ();i++){
            unsigned int o = output[i];
            unsigned int index = i % mod;
            operation = operations[index];
            const std::string operative = operation[0];
            const int operand = std::stoi (operation[1]);
            if(operative == "+"){
                o += operand;
            }else if(operative == "-"){
                o -= operand;
            }else if(operative == "*"){
                o *= operand;
            }else if(operative == "/"){
                o /= operand;
            }else if(operative == "<<"){
                o <<= operand;
            }else if(operative == ">>"){
                o >>= operand;
            }else if(operative == "^"){
                o ^= operand;
            }else if(operative == "%"){
                o %= operand;
            }
            u+=static_cast<unsigned char>(o);
        }
        output = u;
        process(post);
        return output;
    };

public:
    QString name() override {return "9anime";};
    std::string hostUrl() override {return "https://9anime.id";};

    QVector<ShowResponse> search(QString query, int page, int type) override{
        Q_UNUSED(type);
        m_widgetSearched=false;
        m_lastSearch = query.toStdString ();
        return search(m_lastSearch,page);
    };
    QVector<ShowResponse> search(std::string query, int page){
        m_widgetSearched=false;
        m_lastSearch = query;
        std::string url = "https://9anime.pl/filter?keyword="+m_lastSearch+"&page="+std::to_string (page);
        QVector<ShowResponse> animes;
        client.get(url).document()
            .select("//div[@id='list-items']/div[@class='item']").forEach ([&](pugi::xpath_node item) {
                ShowResponse anime;
                auto anchor = item.selectFirst(".//div[@class='ani poster tip']/a");
                auto img = anchor.selectFirst(".//img");
                anime.title = QString::fromStdString (img.attr ("alt").as_string ());
                anime.coverUrl = QString::fromStdString (img.attr ("src").as_string ());
                anime.link = QString::fromStdString (anchor.attr ("href").as_string ());
                anime.latestTxt = QString::fromStdString (item.selectText(".//left/span/span"));
                anime.provider = Providers::e_NineAnime;
                animes.push_back (anime);
            });
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
        return animes;
    };

    ShowResponse loadDetails(ShowResponse show) override {
        CSoup document = client.get(hostUrl() + show.link.toStdString ()).document();
        std::string dataId = document.selectFirst("//div[@id='watch-main']").attr("data-id").as_string ();
        auto episodesUrl=hostUrl()+ "/ajax/episode/list/" + dataId+"?vrf="+encodeVrf (dataId,keys);
        loadEpisodes(&show,episodesUrl);
        pugi::xpath_node element = document.selectFirst("//div[@class='info']");
        show.title = QString::fromStdString(element.selectFirst (".//h1[@class='title d-title']").attr ("data-jp").as_string ());
        //        element.select ("//div[contains(text(), \"Genre\")]/span/a").forEach([&](xpath_node node){
        //            show->genres.push_back (node.node ().child_value ());
        //        });
        std::string desc=element.selectText (".//div[@class='content']");
        Functions::replaceAll (desc,"\n"," ");
        show.description = QString::fromStdString(desc);
        show.status = QString::fromStdString (element.selectText (".//div[contains(text(), \"Status\")]/span"));
        show.releaseDate = element.selectText (".//div[contains(text(), \"Date aired\")]/span");
        show.provider = Providers::e_NineAnime;
        std::string updateTime=document.selectText(".//div[@class='alert next-episode']");
        Functions::replaceAll (updateTime,"\n"," ");
        show.updateTime = QString::fromStdString (updateTime);//trim('×',' ',')','(') ?: "";                                                                                                                                       show.views = element.selectFirst("div:containsOwn(Views) span")!!.text(
        show.rating =QString::fromStdString (element.selectText ("//div[contains(text(), \"Scores\")]/span")).trimmed ();
        //        show->views = QString::fromStdString (element.selectText ("//div[contains(text(), \"Views\")]/span"));
        return show;
    };

    void loadEpisodes(ShowResponse *show,std::string episodeLink){
        NetworkClient::Response episodeData = client.get(episodeLink);
        if (episodeData.body[0] != '{')return;
        //emit error
        std::string resultJson = episodeData.json()["result"].get<std::string>();
        CSoup document{resultJson};
        document.select(R"(//a[@data-ids])").forEach([&](pugi::xpath_node element) {
            Episode episode;
            episode.number = element.attr("data-num").as_int();
            std::string ids = Functions::substringBefore(
                element.attr("data-ids").as_string(), ",");
            QString _name = QString::fromStdString(element.selectText(".//span"));
            QString namePrefix =
                QString("Episode %1").arg(episode.number);
            if (!_name.isEmpty() && _name != namePrefix) {
                episode.title = QString("%1: %2").arg(namePrefix).arg(_name);
            } else {
                episode.title = namePrefix;
            }
            episode.link = hostUrl()+ "/ajax/server/list/" + ids + "?vrf=" + encodeVrf(ids,keys);
            //            episode.link = QString::fromStdString (ids);
            //            qDebug()<<episode.link;
            show->episodes.append(episode);
        });

    }


    std::string getServerData(VideoServer* server, std::string sourceID){
        std::string serverDataUrl =hostUrl ()+"/ajax/server/"+sourceID+"?vrf="+encodeVrf(sourceID, keys);
        auto episodeBody= client.get (serverDataUrl).json ()["result"];//todo add headers
        nlohmann::json skip_data = episodeBody["skip_data"];
        if(!skip_data.empty ()){
            server->skipData = new VideoServer::SkipData;
            server->skipData->introBegin=skip_data["intro_begin"].get<int> ();
            server->skipData->introEnd=skip_data["intro_end"].get<int> ();
            server->skipData->outroBegin=skip_data["outro_begin"].get<int> ();
            server->skipData->outroEnd=skip_data["outro_end"].get<int> ();
        }
        std::string url = decodeVrf(episodeBody["url"].get<std::string>(),keys.decrypt);
        return url.substr (url.find_last_of ("/")+1);
    }

private:
    QVector<ShowResponse> parseAnimes(std::string url){
        CSoup document(client.get(url).json ()["result"]);
        QVector<ShowResponse> animes;
        auto items =document.select("//div[@class='item']");
        items.forEach ([&](pugi::xpath_node node){
            ShowResponse anime;
            pugi::xpath_node anchor = node.selectFirst (".//a[@class='name d-title']");
            anime.title= QString::fromStdString (anchor.node ().child_value ());
            anime.coverUrl=QString::fromStdString( node.selectFirst (".//img").attr ("src").as_string ());

            anime.link=QString::fromStdString( anchor.attr ("href").as_string ());
            anime.latestTxt=QString::fromStdString (node.selectText(".//span[@class='ep-status total']/span"));
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
public:
    QVector<VideoServer> loadServers(Episode *episode) override{
        CSoup document(client.get(episode->link).json ()["result"]);
        QVector<VideoServer> servers;
        document.select("//ul/li").forEach ([&](pugi::xpath_node element){
            VideoServer server;
            std::string serverID = element.attr("data-sv-id").as_string ();
            //            auto linkID = element.attr("data-link-id").as_string ();
            //            server.link = linkID;
            server.link = episode->link;
            if(serverID.compare("41") == 0){
                server.name = "vidstream";
            }
            else if(serverID.compare("28") == 0){
                server.name = "mycloud";
            }
            else if(serverID.compare("43") == 0){
                server.name = "videovard";
            }
            else if(serverID.compare("40") == 0){
                server.name = "streamtape";
            }
            else if(serverID.compare("35") == 0){
                server.name = "mp4upload";
            }
            else if(serverID.compare("44") == 0){
                server.name = "filemoon";
            }
            else{
                // Do nothing or add your code here for handling the else case
            }
            servers.push_back (server);
        });
        return servers;
    };
    void extractSource(VideoServer *server) override{
        if(server->name.compare ("vidstream") == 0){
            auto url = "https://api.consumet.org/anime/9anime/watch/"+server->link+"?server=vizcloud";
            qDebug()<<url;
            auto json = nlohmann::json::parse(client.get ("https://api.consumet.org/anime/9anime/watch/"+server->link+"?server=vizcloud").body);
            server->source=QString::fromStdString (json["sources"][0].get <std::string>());
            //            server->headers={"Referer",json["headers"]["Referer"].get <std::string>(),"User-Agent",json["headers"]["User-Agent"].get <std::string>()};
            //            client.get (json["sources"][0].get <std::string>(),{{"Referer",json["headers"]["Referer"].get <std::string>()},{"User-Agent",json["headers"]["User-Agent"].get <std::string>()}});
        }
    }
public:
    QVector<ShowResponse> fetchMore() override{
        if(m_widgetSearched){
            return widgetSearch (m_lastSearch,++m_currentPage);
        }else{
            return search(m_lastSearch,++m_currentPage);
        }
    };

    // ShowParser interface
protected:
    int providerEnum() override{
        return Providers::e_Nivod;

    }
};

#endif // NINEANIME_H
