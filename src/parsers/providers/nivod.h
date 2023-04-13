#ifndef NIVOD_H
#define NIVOD_H

#include <parsers/showparser.h>
#include <network/client.h>
#include <Functions.hpp>

class Nivod: public ShowParser
{
    bool filterSearched = false;
    std::map<std::string, std::string> lastSearch;
    int providerEnum() override{
        return Providers::e_Nivod;
    }
public:
    Nivod(){};
    QString name() override {return "泥巴影院";}

    std::string hostUrl() override {return "https://www.nivod4.tv/";}

    QVector<ShowResponse> search(QString query, int page, int type) override{
        if(query == ""){
            return QVector<ShowResponse>();
        }
        std::map<std::string, std::string> data = {
            {"keyword", query.toStdString ()},
            {"start", std::to_string((page - 1) * 20)},
            {"cat_id", "1"},
            {"keyword_type", "0"}
        };
        m_currentPage=page;
        return search(data);
    };

    QVector<ShowResponse> search(std::map<std::string, std::string> data){
        lastSearch=data;
        filterSearched=false;
        std::string response = callAPI("https://api.nivodz.com/show/search/WEB/3.2", data);
        QVector<ShowResponse> results = showsFromJsonArray (nlohmann::json::parse(response)["list"]);
        return results;
    };

    QVector<ShowResponse> filterSearch(int page, std::string sortBy,std::string channel,std::string regionId = "0",std::string langId="0",std::string yearRange=" ") {
        m_currentPage=page;
        std::map<std::string, std::string> data = {
            {"sort_by", sortBy},
            {"channel_id", channel},
            {"show_type_id", "0"},
            {"region_id", "0"},
            {"lang_id", "0"},
            {"year_range", " "},
            {"start", std::to_string((m_currentPage - 1) * 20)}
        };
        return filterSearch(data);
    }

    QVector<ShowResponse> filterSearch(std::map<std::string, std::string> data) {
        lastSearch=data;
        filterSearched=true;
        std::string response = callAPI("https://api.nivodz.com/show/filter/WEB/3.2", data);
        QVector<ShowResponse> results = showsFromJsonArray(nlohmann::json::parse(response)["list"]);
        return results;
    }

    QVector<ShowResponse> popular(int page, int type)override {
        return filterSearch(page,"1",channelId[static_cast<TvType>(type)]);
    }

    QVector<ShowResponse> latest(int page, int type)override {
        return filterSearch(page,"4",channelId[static_cast<TvType>(type)]);
    }

    ShowResponse loadDetails(ShowResponse show) override{
        auto response = callAPI("https://api.nivodz.com/show/detail/WEB/3.2", {{"show_id_code", show.link.toStdString ()}});
        auto entity = nlohmann::json::parse(response)["entity"];
        //        qDebug()<<QString::fromStdString (response);
        show.description = QString::fromStdString (entity["showDesc"].get<std::string>());
        if(entity.contains ("episodesUpdateDesc")&&!entity.at ("episodesUpdateDesc").is_null ()){
            show.updateTime = QString::fromStdString (entity["episodesUpdateDesc"].get<std::string>());
        }
        if(!entity["showTypeName"].is_null ()){
            show.genres += QString::fromStdString(entity["showTypeName"].get<std::string>());
        }

        //        auto actors = QString::fromStdString(entity["actors"].get<std::string>());

        if(!entity["hot"].is_null ()){
            show.views = entity["hot"].get<int>();
        }
        if(!entity["rating"].is_null ()){
            show.rating = QString::number ((entity["rating"].get<int>())/10);
        }
        for (auto& item : entity["plays"].items()){
            Episode ep;
            auto episode = item.value ();
            ep.number = episode["episodeId"].get<int>();
            ep.title = QString::fromStdString (episode["displayName"].get<std::string>());
            ep.link = show.link.toStdString ()+ "&"+episode["playIdCode"].get<std::string>();
            show.episodes.append(ep);
        }
        return show;
    };

    QVector<VideoServer> loadServers(Episode *episode) override{
        return QVector<VideoServer>{VideoServer{"default",episode->link,{{"referer","https://www.nivod.tv/"}}}};
    };

    void extractSource(VideoServer *server) override{
        auto codes = Functions::split(server->link,'&');
        std::map<std::string, std::string> data = {
            {"play_id_code", codes[1]},
            {"show_id_code", codes[0]},
            {"oid","1"},
            {"episode_id","0"}
        };
        auto playUrl=nlohmann::json::parse(callAPI("https://api.nivodz.com/show/play/info/WEB/3.3", data))["entity"]["plays"][0]["playUrl"].get <std::string>();//video quality
        server->source = QString::fromStdString (playUrl);
    };

    QVector<ShowResponse> fetchMore() override{
        lastSearch["start"] = std::to_string((++m_currentPage - 1) * 20);
        if(filterSearched){
            return filterSearch(lastSearch);
        }else{
            return search(lastSearch);
        }
    }
private:
    const std::string _HOST_CONFIG_KEY = "2x_Give_it_a_shot";
    const std::string _bp_app_version = "1.0";
    const std::string _bp_platform = "3";
    const std::string _bp_market_id = "web_nivod";
    const std::string _bp_device_code = "web";
    const std::string _bp_versioncode = "1";
    const std::string _QUERY_PREFIX = "__QUERY::";
    const std::string _BODY_PREFIX = "__BODY::";
    const std::string _SECRET_PREFIX = "__KEY::";
    const std::string _oid = "8c387951eff11000ef5216f0e7cdca70956c120f20e90d2f";
    std::string _mts = "1679142478147";//QString::number(QDateTime::currentMSecsSinceEpoch()).toStdString ();
    const std::map<std::string, std::string> queryMap = {
        {"_ts", _mts},
        {"app_version", _bp_app_version},
        {"device_code", _bp_device_code},
        {"market_id", _bp_market_id},
        {"oid", _oid},
        {"platform", _bp_platform},
        {"versioncode", _bp_versioncode}
    };
    const std::map<std::string, std::string> mudvodHeaders = {{"referer", "https://www.nivod4.tv"}};
    const QMap<TvType, std::string> channelId = {
        {TvType::Movie, "1"},
        {TvType::TvSeries, "2"},
        {TvType::Reality, "3"},
        {TvType::Anime, "4"},
        {TvType::Documentary, "6"}
    };

    std::string createSign(const std::map<std::string, std::string> & bodyMap, const std::string& secretKey = "2x_Give_it_a_shot"){
        std::stringstream ss;
        std::string signQuery = _QUERY_PREFIX;
        for(auto const& [key,value]:queryMap){
            signQuery += key+"="+value+"&";
        }

        ss << _BODY_PREFIX;
        for (const auto& [key, value] : bodyMap) {
            ss << key << '=' << value << '&';
        }

        std::string input = signQuery + ss.str() + _SECRET_PREFIX + secretKey;
        return Functions::MD5(input);
    }

    std::string decryptedByDES(const std::string& input){
        std::string key = "diao.com";
        std::vector<byte> keyBytes(key.begin(), key.end());
        std::vector<byte> inputBytes;
        for (size_t i = 0; i < input.length(); i += 2) {
            byte byte = static_cast<unsigned char>(std::stoi(input.substr(i, 2), nullptr, 16));
            inputBytes.push_back(byte);
        }

        size_t length = inputBytes.size();
        size_t padding = length % 8 == 0 ? 0 : 8 - length % 8;
        inputBytes.insert(inputBytes.end(), padding, 0);

        std::vector<byte> outputBytes(length + padding, 0);
        CryptoPP::ECB_Mode<CryptoPP::DES>::Decryption decryption(keyBytes.data(), keyBytes.size());
        CryptoPP::ArraySink sink(outputBytes.data(), outputBytes.size());
        CryptoPP::ArraySource source(inputBytes.data(), inputBytes.size(), true, new CryptoPP::StreamTransformationFilter(decryption, new CryptoPP::Redirector(sink)));
        std::string decrypted(outputBytes.begin(), outputBytes.end());
        size_t pos = decrypted.find_last_of('}');
        if (pos != std::string::npos) {
            decrypted = decrypted.substr(0, pos + 1);
        }
        return decrypted;
    }

    QVector<ShowResponse> showsFromJsonArray(const nlohmann::json& showList,int type=0) {
        QVector<ShowResponse> results;
        for (auto& el : showList.items())
        {
            auto item=el.value ();
            ShowResponse show;

            TvType tvType = TvType::Anime;
            std::string channelName = item["channelName"].get<std::string>();
            if ( channelName== "电影") {
                tvType = TvType::Movie;
            } else if (channelName == "电视剧") {
                tvType = TvType::TvSeries;
            } else if (channelName == "综艺") {
                tvType = TvType::Reality;
            } else if (channelName == "动漫") {
                tvType = TvType::Anime;
            } else if (channelName == "纪录片") {
                tvType = TvType::Documentary;
            }
            show.type=to_underlying(tvType);
            std::string showImg = item["showImg"].get<std::string>();
            if(showImg.back ()!='g')showImg+="_300x400.jpg";

            //            if(item["showTitle"].get<std::string>().find ("假面骑士"));
            show.title = QString::fromStdString (item["showTitle"].get<std::string>());
            show.coverUrl = QString::fromStdString (showImg);
            show.link = QString::fromStdString (item["showIdCode"].get<std::string>());
            if(!item["episodesTxt"].is_null ()){
                show.latestTxt = QString::fromStdString (item["episodesTxt"].get<std::string>());
            }
            show.provider=Providers::e_Nivod;
            if(!item["postYear"].is_null ()){
                show.releaseDate = QString::number(item["postYear"].get<int>());
            }

            //            Status status = Status::Completed;
            //            if (showResponse.latestTxt.contains("更新")) {
            //                status = Status::Ongoing;
            //            }

            results.push_back (show);
        }
        m_canFetchMore=!results.isEmpty ();
        return results;
    }

    std::string callAPI(const std::string& url, const std::map<std::string, std::string>& data){
        std::string sign = createSign(data);
        std::string postUrl =url+"?_ts="+_mts+"&app_version=1.0&platform=3&market_id=web_nivod&device_code=web&versioncode=1&oid="+_oid+"&sign="+sign;
        auto response=client.post(postUrl,mudvodHeaders,data).body;
        //        qDebug()<<QString::fromStdString (postUrl);
        //        qDebug()<<QString::fromStdString (response);
        while(response[0]=='<'){
            response=client.post(postUrl,mudvodHeaders,data).body;
            qDebug()<<"Bad Response";
        }
        try{
            return decryptedByDES(response);
        }catch(std::exception& e){
            qDebug()<<"Bad Input";
            qDebug()<<QString::fromStdString (postUrl);
            qDebug()<<QString::fromStdString (response);
        }
        return "";

    }

};

#endif // NIVOD_H
