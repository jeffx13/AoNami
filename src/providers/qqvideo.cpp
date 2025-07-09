#include "qqvideo.h"

#include <QCoreApplication>
#include <QProcess>

#include <QUrlQuery>

QList<ShowData> QQVideo::search(Client *client, const QString &query, int page, int type)
{
    return {};
}

QList<ShowData> QQVideo::popular(Client *client, int page, int typeIndex)
{

    // # TV series 75 most popular, 79 latest, 85 highest rated
    // # Movies 75 most popular, 83 latest, 81 highest rated
    return filterSearch(client, 75, page, typeIndex);
}

QList<ShowData> QQVideo::latest(Client *client, int page, int typeIndex)
{
    int sortBy = typeIndex == 2 ? 79 : 83; // 79 is TV series, 83 is movies
    return filterSearch(client, sortBy, page, typeIndex);
}

QList<ShowData> QQVideo::filterSearch(Client *client, int sortBy, int page, int type) {
    QString channelId = QString::number(types[type]);
    int resultsPerPage = 12;

    QMap<QString, QString> headers {
        { "accept", "application/json" },
        { "accept-language", "en-GB,en;q=0.9,zh-CN;q=0.8,zh;q=0.7" },
        { "cache-control", "no-cache" },
        { "content-type", "application/json" },
        { "origin", "https://v.qq.com" },
        { "pragma", "no-cache" },
        { "priority", "u=1, i" },
        { "referer", "https://v.qq.com/" },
        { "sec-ch-ua", "\"Google Chrome\";v=\"137\", \"Chromium\";v=\"137\", \"Not/A)Brand\";v=\"24\"" },
        { "sec-ch-ua-mobile", "?0" },
        { "sec-ch-ua-platform", "\"Windows\"" },
        { "sec-fetch-dest", "empty" },
        { "sec-fetch-mode", "cors" },
        { "sec-fetch-site", "same-site" },
        { "user-agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36" }
    };


    QJsonObject pageParams {
        { "channel_id", channelId },
        { "filter_params", QString("sort=%1&attr=-1&itype=-1&ipay=-1&iarea=-1&iyear=-1&theater=-1&award=-1").arg(sortBy) },
        { "page_type", "operation" },
        { "page_id", "channel_list" }
    };

    QJsonObject pageContext {
        { "_ctrl_page_index", QString::number(page) },
        { "_ctrl_showed_module_num", QString::number(page) },
        { "_ds_cli_6970df954e7a9803_poster_offset", QString::number(page * resultsPerPage) },
        { "_ds_cli_6970df954e7a9803_poster_size", QString::number(resultsPerPage) },
        { "_merger_mod_cnt", QString::number(page) },
        { "page_index", QString::number(page) },
        { "sdk_page_ctx", QString("{\"page_offset\":%1,\"page_size\":5,\"used_module_num\":%2}").arg(page).arg(page) },
        { "video_un_page_index", QString::number(page) }
    };

    QJsonObject jsonData {
        { "page_params", pageParams },
        { "page_context", pageContext }
    };
    QJsonDocument doc(jsonData);
    QByteArray payload = doc.toJson();
    QString endpoint = "https://pbaccess.video.qq.com/trpc.multi_vector_layout.mvl_controller.MVLPageHTTPService/getMVLPage?&vplatform=2";
    auto showList = client->post(endpoint, payload, headers).toJsonObject()
                        ["data"].toObject()["modules"].toObject()
                                ["normal"].toObject()["cards"].toArray()[0].toObject()
                                ["children_list"].toObject()["poster_card"].toObject()["cards"].toArray();

    QList<ShowData> shows;
    for (int i = 0; i < showList.size(); i++) {
        auto showJson = showList[i].toObject()["params"].toObject();
        QString link = showJson["cid"].toString();
        QString title =showJson["title"].toString();
        QString coverUrl =showJson["new_pic_vt"].toString();
        QString latestText =showJson["timelong"].toString();
        shows.emplaceBack(title, link, coverUrl, this, latestText);
    }

    return shows;
}



int QQVideo::loadDetails(Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const
{
    QJsonObject pageParams {
        { "req_from", "web" },
        { "cid", show.link },
        { "vid", "" },
        { "lid", "" },
        { "page_type", "detail_operation" },
        { "page_id", "detail_page_introduction" }
    };

    QJsonObject jsonData {
        { "page_params", pageParams },
        { "has_cache", 1 }
    };

    QJsonDocument doc(jsonData);
    QByteArray payload = doc.toJson();



    auto response = client->post(
                              "https://pbaccess.video.qq.com/trpc.universal_backend_service.page_server_rpc.PageServer/GetPageData?video_appid=3000010&vversion_name=8.2.98&vversion_platform=2",
                              payload, headers).toJsonObject();

    auto item_params = response["data"].toObject()["module_list_datas"].toArray()[0].toObject()
                           ["module_datas"].toArray()[0].toObject()["item_data_lists"].toObject()["item_datas"].toArray()[0].toObject()
                                   ["item_params"].toObject();
    show.views = item_params["hot_num"].toString();
    show.releaseDate = item_params["cover_year"].toString();
    show.updateTime = item_params["holly_online_time"].toString() + "。 " + item_params["update_notify_desc"].toString();
    show.description = item_params["cover_description"].toString();
    auto genres = item_params["main_genres"].toString();
    auto totalEpisodes = item_params["episode_all"].toString().toInt();

    if (getEpisodeCountOnly) {
        return totalEpisodes;
    }

    // print(f"Episode All: {episode_all}, Hotval: {hot_num}, Cover Description: {cover_description}")
    // qDebug() << "Episode All:" << totalEpisodes
    //          << ", Hotval:" << item_params["hot_num"].toString()
    //          << ", Cover Description:" << item_params["cover_description"].toString();

    QString endpoint = "https://pbaccess.video.qq.com/trpc.universal_backend_service.page_server_rpc.PageServer/GetPageData?video_appid=3000010&vversion_name=8.2.96&vversion_platform=2";

    if (fetchPlaylist) {
        auto totalPages = (totalEpisodes + 100 - 1) / 100;
        int pageSize = 100;

        for (int i = 0; i < totalPages; ++i) {
            int episodeBegin = pageSize * i + 1;
            int episodeEnd = pageSize * (i + 1);
            QString pageContext = QString("episode_begin=%1&episode_end=%2&episode_step=1&page_num=0&page_size=100")
                                      .arg(episodeBegin)
                                      .arg(episodeEnd);

            QJsonObject pageParams {
                { "req_from", "web_vsite" },
                { "page_id", "vsite_episode_list" },
                { "page_type", "detail_operation" },
                { "id_type", "1" },
                { "page_size", "" },
                { "cid", show.link },
                { "vid", "" },
                { "lid", "" },
                { "page_num", "" },
                { "page_context", pageContext },
                { "detail_page_type", "1" }
            };

            QJsonObject jsonData {
                { "page_params", pageParams },
                { "has_cache", 1 }
            };

            QJsonDocument doc(jsonData);
            QByteArray payload = doc.toJson();

            // Send POST request
            auto data = client->post(endpoint, payload, headers).toJsonObject()["data"].toObject();

            // Navigate to episodes
            if (!data.contains("module_list_datas")) continue;
            auto module_list_datas = data["module_list_datas"].toArray();
            if (module_list_datas.isEmpty()) continue;

            auto itemDatas = module_list_datas[0].toObject()
                                 ["module_datas"].toArray()[0].toObject()
                                         ["item_data_lists"].toObject()
                                         ["item_datas"].toArray();

            bool isTrailer = false;

            for (const QJsonValue &episodeValue : std::as_const(itemDatas)) {
                auto episodeObj = episodeValue.toObject();
                auto params = episodeObj["item_params"].toObject();
                isTrailer = params["is_trailer"].toString() == "1";

                QString title = params["video_subtitle"].toString();
                // QString playTitle = params["play_title"].toString();
                QString link = params["vid"].toString();
                auto episodeNumber = params["title"].toString().toFloat();
                show.addEpisode(0, episodeNumber, QString("%1/%2").arg(show.link, link), title);
            }
            if (isTrailer) break; // last page
        }
    }

    // auto rating = result["rating"].toObject();
    // show.score = QString("%1 (%2)").arg(QString::number(rating["score"].toDouble()), QLocale::system().toString(rating["count"].toInt()));

    return true;
}

QList<VideoServer> QQVideo::loadServers(Client *client, const PlaylistItem *episode) const
{
    auto coverVideo = episode->link.split("/");
    auto cover = coverVideo.first();
    auto vid = coverVideo.last();
    QString referrer = QString("https://v.qq.com/x/cover/%1.html").arg(coverVideo.first());
    QString vurl = QString("https://v.qq.com/x/cover/%1/%2.html").arg(coverVideo.first(), coverVideo.last());


    QStringList args {jsFile, "10201", appVer, coverVideo.last(), vurl, referrer};
    QStringList parts = getCkey(args);
    QString ckey = parts[0];
    QString tm = parts[1];
    QString guid = parts[2];
    QString flowid = parts[3];
    QString definition = "uhd";

    // gLog() << name() << "vid:" << vid
    //        << ", ckey:" << ckey
    //        << ", tm:" << tm
    //        << ", guid:" << guid
    //        << ", flowid:" << flowid;

    QMap<QString, QString> vinfoparamsMap {
        { "otype", "ojson"},
        { "isHLS", "1" },
        { "charge", "0" },
        { "fhdswitch", "0" },
        { "show1080p", "1" },
        { "defnpayver", "7" },
        { "sdtfrom", "v1010" },
        { "host", "v.qq.com" },
        { "vid", vid },
        { "defn", definition },
        { "platform", "10201" },
        { "appVer", appVer },
        { "refer", referrer },
        { "ehost", vurl },
        { "logintoken", m_loginToken },
        { "encryptVer", encryptVer },
        { "guid", guid },
        { "flowid", flowid },
        { "tm", tm },
        { "cKey", ckey },
        { "dtype", "3" },
        {"drm", "40"}
    };

    QString vinfoparams = buildParamString(vinfoparamsMap);
    QByteArray payload = QJsonDocument(QJsonObject{{"buid", "vinfoad"},
                                                {"vinfoparam", vinfoparams}})
                          .toJson();
    auto data = client->post(apiUrl, payload, headers).toJsonObject();
    if (!data.contains("vinfo") ) {
        oLog() << name() << data["msg"].toString();
        return {};
    }
    QJsonObject vinfo = QJsonDocument::fromJson(data["vinfo"].toString().toUtf8()).object();

    auto formats = vinfo["fl"].toObject()["fi"].toArray();
    QList<VideoServer> servers;

    // auto vfilename = data["vl"].toObject()["vi"].toArray()[0].toObject()["fn"].toString();
    // auto vfn = vfilename.split(".");
    // auto ext = vfn.last();
    // auto fmt_prefix = vfn.count() == 3 ? vfn[1][0] : 'p';

    // auto vfilefs = data["vl"].toObject()["vi"].toArray()[0].toObject()["fs"].toInt();
    // auto orig_format_id = -1;
    // auto fc = data["vl"].toObject()["vi"].toArray()[0].toObject()["fc"]; //
    // auto keyidsplit = data["vl"].toObject()["vi"].toArray()[0].toObject()["keyid"].toString().split(".");

    for (int i = 0; i < formats.size(); i++) {
        auto format = formats[i].toObject();
        auto cname = format["cname"].toString();
        auto formatName = format["name"].toString();
        auto id = format["id"].toInt();
        auto fs = format["fs"].toInt();

        // if (vfilefs == fs) {
        //     orig_format_id = id;
        // }

        //
        oLog() << name() << formatName << episode->link;
        servers.emplaceBack(formatName, episode->link);
    }
    // auto format_id = 100; // TODO

    // auto vfmt_new = fmt_prefix + QString::number(format_id % 10000);

    // if (orig_format_id == -1) {
    //     orig_format_id = formats[0].toObject()["id"].toInt();
    // }

    // keyidsplit[1] = QString::number(orig_format_id);
    // QString keyid = keyidsplit.join(".");
    // int max_fc = 80;

    // for (int idx = 1; idx <= max_fc; idx++) {
    //     auto keyid_new_split = keyid.split('.');
    //     keyid_new_split[0] = vfn[0];
    //     QString keyid_new;

    //     if (keyid_new_split.count() == 3) {
    //         keyid_new_split[1] = vfmt_new;
    //         keyid_new_split[2] = QString::number(idx);
    //         keyid_new = keyid_new_split.join(".");
    //     } else {
    //         if (keyid_new[1].toInt() != orig_format_id) {
    //             if (vfn.count() == 3) {
    //                 vfn[1] = vfn[1][0] + QString::number(orig_format_id);
    //             } else {
    //                 vfn.insert(1, vfmt_new);
    //             }
    //         }
    //         keyid_new = vfn.join(".");
    //     }
    // }



    return servers;
}




PlayItem QQVideo::extractSource(Client *client, VideoServer &server)
{
    // require vid, format id
    rLog() << name() << server.name << server.link;
    auto coverVideo = server.link.split("/");
    auto cover = coverVideo.first();
    auto vid = coverVideo.last();
    QString referrer = QString("https://v.qq.com/x/cover/%1.html").arg(coverVideo.first());
    QString vurl = QString("https://v.qq.com/x/cover/%1/%2.html").arg(coverVideo.first(), coverVideo.last());

    QStringList args {jsFile, "10201", appVer, coverVideo.last(), vurl, referrer};
    QStringList parts = getCkey(args);
    QString ckey = parts[0];
    QString tm = parts[1];
    QString guid = parts[2];
    QString flowid = parts[3];

    auto definition = server.name;
    QMap<QString, QString> vinfoparamsMap {
        { "otype", "ojson"},
        { "isHLS", "1" },
        { "charge", "0" },
        { "fhdswitch", "0" },
        { "show1080p", "1" },
        { "defnpayver", "7" },
        { "sdtfrom", "v1010" },
        { "host", "v.qq.com" },
        { "vid", vid },
        { "defn", definition },
        { "platform", "10201" },
        { "appVer", appVer },
        { "refer", referrer },
        { "ehost", vurl },
        { "vid", vid },
        { "logintoken", m_loginToken},
        { "encryptVer", encryptVer },
        { "guid", guid },
        { "flowid", flowid },
        { "tm", tm },
        { "cKey", ckey },
        { "dtype", "3" },
        {"drm", "40"}
    };

    QString vinfoparams = buildParamString(vinfoparamsMap);
    QByteArray payload = QJsonDocument(QJsonObject{{"buid", "vinfoad"},
                                                   {"vinfoparam", vinfoparams}})
                             .toJson();
    QJsonObject data = client->post(apiUrl, payload, headers).toJsonObject();


    // qDebug() << "Response from API:" << data;
    if (!data.contains("vinfo") ) {
        oLog() << name() << data["msg"].toString();
        return {};
    }
    QJsonObject vinfo = QJsonDocument::fromJson(data["vinfo"].toString().toUtf8()).object();
    // gLog() << name() << QJsonDocument(vinfo).toJson(QJsonDocument::Indented);;
    auto sources = vinfo["vl"].toObject()["vi"].toArray()[0].toObject()["ul"].toObject()["ui"].toArray();

    PlayItem playItem;
    for (int i = 0; i < sources.size(); i++) {
        auto video = sources[i].toObject();
        auto url = video["url"].toString();
        mLog() << name() << url;
        playItem.videos.emplaceBack(url);
    }
    return playItem;
}

QString QQVideo::buildParamString(QMap<QString, QString> params) const {
    QString out;
    QUrlQuery vinfoparam;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        // out += QString("%1=%2").arg(it.key(), it.value());
        vinfoparam.addQueryItem(it.key(), it.value());
    }
    out = vinfoparam.toString(QUrl::FullyEncoded);

    return out;
}



QStringList QQVideo::getCkey(QStringList &args) const {
    QString nodePath = "C:\\nvm4w\\nodejs\\node.exe";
    QProcess nodeProc;
    nodeProc.setProgram (nodePath);
    nodeProc.setArguments(args);
    nodeProc.start();

    // Check if process started successfully
    if (!nodeProc.waitForStarted(5000)) {
        qWarning() << "Failed to start Node.js process:" << nodeProc.errorString();
    }

    // Wait for process to finish (increased timeout)
    if (!nodeProc.waitForFinished(10000)) {
        qWarning() << "Node.js process timeout or failed to finish:" << nodeProc.errorString();
        nodeProc.kill();
    }
    if (nodeProc.exitCode() != 0) {
        qWarning() << "Node.js process exited with code:" << nodeProc.exitCode();
    }
    // Read both stdout and stderr
    QByteArray stdoutData = nodeProc.readAllStandardOutput();
    // QByteArray stderrData = nodeProc.readAllStandardError();
    nodeProc.close();

    QList<QByteArray> parts = stdoutData.trimmed().split(' ');
    QStringList result;
    for (const QByteArray &part : parts) {
        result << QString(part);
    }
    return result;
}
