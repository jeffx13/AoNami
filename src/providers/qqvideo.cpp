#include "qqvideo.h"
#include "app/settings.h"

#include <QCoreApplication>
#include <QProcess>
#include <QUrlQuery>
#include <QUrl>
#include <QtConcurrent>
#include <algorithm>

QQVideo::QQVideo(QObject *parent) : ShowProvider(parent) {
    // Read from Settings singleton (INI)
    m_loginToken = Settings::instance().getString(QStringLiteral("qqvideo/logintoken"));
}

QList<ShowData> QQVideo::search(Client *client, const QString &query, int page, int type)
{
    return {};
}

QList<ShowData> QQVideo::popular(Client *client, int page, int typeIndex)
{
    if (typeIndex < 0 || typeIndex >= channelIds.size()) return {};
    // 75 = most popular across channels
    return filterSearch(client, 75, page, typeIndex);
}

QList<ShowData> QQVideo::latest(Client *client, int page, int typeIndex)
{
    if (typeIndex < 0 || typeIndex >= channelIds.size()) return {};
    // Different sort IDs for latest content by type
    // 电影 (Movies): 83, 电视剧 (TV Series): 79, 综艺 (Variety): 23, 动漫 (Anime): 23
    QList<int> latestSortIds = {83, 79, 23, 23};
    int sortBy = latestSortIds[typeIndex];
    
    return filterSearch(client, sortBy, page, typeIndex);
}

QList<ShowData> QQVideo::filterSearch(Client *client, int sortBy, int page, int type) {
    QList<ShowData> shows;
    if (type < 0 || type >= channelIds.size()) return shows;
    int channelId = channelIds[type];

    // Build body with sort filter and page_context for pagination
    QJsonObject pageParams{
        {"channel_id", QString::number(channelId)},
        {"filter_params", QString("sort=%1").arg(sortBy)},
        {"page_id", "channel_list"},
        {"page_type", "operation"}
    };

    QString pageStr = QString::number(page);
    QString posterOffset = QString::number(page * 12);
    QString posterSize = QString::number(12);
    QString sdkPageCtx = QString("{\"page_offset\":%1,\"page_size\":5,\"used_module_num\":%2}")
                             .arg(pageStr, pageStr);
    QJsonObject pageContext{
        {"_ctrl_page_index", pageStr},
        {"_ctrl_showed_module_num", pageStr},
        {"_ds_cli_6970df954e7a9803_poster_offset", posterOffset},
        {"_ds_cli_6970df954e7a9803_poster_size", posterSize},
        {"_merger_mod_cnt", pageStr},
        {"page_index", pageStr},
        {"sdk_page_ctx", sdkPageCtx},
        {"video_un_page_index", pageStr}
    };

    QJsonObject body{{"page_params", pageParams}, {"page_context", pageContext}};
    auto data = QJsonDocument(body).toJson(QJsonDocument::Compact);

    auto resp = client->post("https://pbaccess.video.qq.com/trpc.multi_vector_layout.mvl_controller.MVLPageHTTPService/getMVLPage?&vplatform=2", data, headers);
    QJsonObject root = resp.toJsonObject();
    if (root.isEmpty()) return shows;

    auto dataObj = root.value("data").toObject();
    auto modules = dataObj.value("modules").toObject();
    auto normal = modules.value("normal").toObject();
    auto cards = normal.value("cards").toArray();
    if (cards.isEmpty()) return shows;
    auto first = cards.at(0).toObject();
    auto childrenList = first.value("children_list").toObject();
    auto posterCard = childrenList.value("poster_card").toObject();
    auto posterCards = posterCard.value("cards").toArray();
    Q_FOREACH(const auto &v, posterCards) {
        QJsonObject card = v.toObject();
        QJsonObject params = card.value("params").toObject();
        QString cid = params.value("cid").toString();
        QString title = params.value("title").toString();
        QString cover = params.value("new_pic_vt").toString();
        if (cid.isEmpty() || title.isEmpty()) continue;
        // Use cover page as link id; we can construct URL later
        shows.emplaceBack(title, cid, cover, this, "", m_typeIndexToType.value(type, ShowData::NONE));
    }
    return shows;
}




int QQVideo::loadShow(Client *client, ShowData &show, bool getEpisodeCountOnly, bool getPlaylist, bool getInfo) const
{
    // Build detail request to fetch info
    if (getInfo) {
        QJsonObject pageParams{
            {"req_from", "web"},
            {"cid", show.link},
            {"vid", ""},
            {"lid", ""},
            {"page_type", "detail_operation"},
            {"page_id", "detail_page_introduction"}
        };
        QJsonObject body{{"page_params", pageParams}, {"has_cache", 1}};
        auto data = QJsonDocument(body).toJson(QJsonDocument::Compact);
        auto resp = client->post("https://pbaccess.video.qq.com/trpc.universal_backend_service.page_server_rpc.PageServer/GetPageData?video_appid=3000010&vversion_name=8.2.98&vversion_platform=2", data, headers);
        QJsonObject root = resp.toJsonObject();
        auto dataObj = root.value("data").toObject();
        auto modList = dataObj.value("module_list_datas").toArray();
        if (!modList.isEmpty()) {
            auto modDataArr = modList.at(0).toObject().value("module_datas").toArray();
            if (!modDataArr.isEmpty()) {
                auto itemDatas = modDataArr.at(0).toObject().value("item_data_lists").toObject().value("item_datas").toArray();
                if (!itemDatas.isEmpty()) {
                    auto itemParams = itemDatas.at(0).toObject().value("item_params").toObject();
                    show.description = itemParams.value("cover_description").toString();
                    show.views = itemParams.value("hot_num").toString();
                    show.releaseDate = itemParams.value("hollywood_online").toString();
                    auto img = itemParams.value("new_pic_vt").toString();
                    if (!img.isEmpty()) show.coverUrl = img;
                    auto genresStr = itemParams.value("main_genres").toString();
                    if (!genresStr.isEmpty()) show.genres = genresStr.split("/", Qt::SkipEmptyParts);
                    show.updateTime = itemParams.value("update_notify_desc").toString();
                }
            }
        }
    }

    // Fetch playlist (episodes) via paginated API with parallel page loading
    int totalCount = 0;
    const int episodeStep = 30;

    struct Ep { int num; QString cid; QString vid; QString title; };
    QList<Ep> collected;
    QString showCid = show.link;

    auto fetchPage = [this, showCid, episodeStep](Client *baseClient, int episodeBegin) -> QList<Ep> {
        QList<Ep> result;
        if (episodeBegin <= 0) return result;
        Client subClient = *baseClient;
        int episodeEnd = episodeBegin + episodeStep - 1;
        int pageSize = episodeEnd - episodeBegin + 1;

        QString pageContext = QString(
            "chapter_name=&cid=%1&detail_page_type=1&episode_begin=%2&episode_end=%3&episode_step=30&filter_rule_id=&id_type=1&is_nocopyright=false&is_skp_style=false&lid=&list_page_context=&mvl_strategy_id=&need_tab=1&order=&page_num=1&page_size=%4&req_from=web_vsite&req_from_second_type=&req_type=0&siteName=&tab_type=1&title_style=&ui_type=null&un_strategy_id=13dc6f30819942eb805250fb671fb082&watch_together_pay_status=0&year="
        ).arg(showCid).arg(episodeBegin).arg(episodeEnd).arg(pageSize);

        QJsonObject pageParams{
            {"req_from", "web_vsite"},
            {"page_id", "vsite_episode_list"},
            {"page_type", "detail_operation"},
            {"id_type", "1"},
            {"page_size", ""},
            {"cid", showCid},
            {"vid", ""},
            {"lid", ""},
            {"page_num", ""},
            {"page_context", pageContext},
            {"detail_page_type", "1"}
        };
        QJsonObject reqBody{{"page_params", pageParams}, {"has_cache", 1}};
        auto data = QJsonDocument(reqBody).toJson(QJsonDocument::Compact);

        try {
            auto resp = subClient.post(
                "https://pbaccess.video.qq.com/trpc.universal_backend_service.page_server_rpc.PageServer/GetPageData?video_appid=3000010&vversion_name=8.2.96&vversion_platform=2",
                data,
                headers
            );
            QJsonObject root = resp.toJsonObject();
            auto dataObj = root.value("data").toObject();
            auto modList = dataObj.value("module_list_datas").toArray();
            if (modList.isEmpty()) return result;
            auto modDataArr = modList.at(0).toObject().value("module_datas").toArray();
            if (modDataArr.isEmpty()) return result;
            auto itemDatas = modDataArr.at(0).toObject().value("item_data_lists").toObject().value("item_datas").toArray();
            if (itemDatas.isEmpty()) return result;

            int offset = 0;
            Q_FOREACH(const auto &v, itemDatas) {
                QJsonObject item = v.toObject();
                QString vid = item.value("item_id").toString();
                QJsonObject itemParams = item.value("item_params").toObject();
                QString unionTitle = itemParams.value("union_title").toString();
                if (unionTitle.isEmpty()) unionTitle = itemParams.value("title").toString();
                QString cid = itemParams.value("cid").toString();
                if (cid.isEmpty()) continue;
                Ep ep{ episodeBegin + offset, cid, vid, unionTitle };
                result.push_back(ep);
                offset++;
            }
        } catch (...) {
            return {};
        }
        return result;
    };

    const int maxPagesPerBatch = 6;
    int currentBegin = 1;
    bool hasMore = true;
    while (hasMore) {
        QList<QFuture<QList<Ep>>> futures;
        for (int i = 0; i < maxPagesPerBatch; ++i) {
            int begin = currentBegin + i * episodeStep;
            futures.push_back(QtConcurrent::run([&fetchPage, client, begin]() {
                return fetchPage(client, begin);
            }));
        }

        hasMore = false;
        for (int i = 0; i < futures.size(); ++i) {
            futures[i].waitForFinished();
            QList<Ep> pageEps;
            try { pageEps = futures[i].result(); } catch (...) { pageEps.clear(); }
            if (!pageEps.isEmpty()) {
                totalCount += pageEps.size();
                if (getPlaylist) collected.append(pageEps);
                if (pageEps.size() == episodeStep) hasMore = true;
            }
        }
        currentBegin += maxPagesPerBatch * episodeStep;
    }

    if (getPlaylist) {
        if (collected.isEmpty()) {
            // Fallback: add detail page as single episode if none retrieved
            QString url = QString("https://v.qq.com/x/cover/%1.html").arg(show.link);
            show.addEpisode(0, 1, url, show.title);
            totalCount = 1;
        } else {
            std::sort(collected.begin(), collected.end(), [](const Ep &a, const Ep &b){ return a.num < b.num; });
            Q_FOREACH(const auto &ep, collected) {
                QString epUrl = QString("https://v.qq.com/x/cover/%1/%2.html").arg(ep.cid, ep.vid);
                show.addEpisode(0, ep.num, epUrl, ep.title);
            }
        }
    }

    return totalCount > 0 ? totalCount : -1;
}

QList<VideoServer> QQVideo::loadServers(Client *client, const PlaylistItem *episode) const
{
    Q_UNUSED(client);
    QList<VideoServer> servers {
                               VideoServer{"fhd", episode->link},
                               VideoServer{"shd", episode->link},
                               VideoServer{"hd", episode->link},
                               VideoServer{"sd", episode->link},
                               };
    return servers;
}




PlayInfo QQVideo::extractSource(Client *client, VideoServer &server)
{
    Q_UNUSED(client);
    PlayInfo info;
    info.headers = headers;

    // Parse cid and vid from episode page URL (https://v.qq.com/x/cover/{cid}/{vid}.html)
    QString pageUrl = server.link;
    QUrl url(pageUrl);
    QStringList segments = url.path().split('/', Qt::SkipEmptyParts);
    QString cid;
    QString vid;
    if (segments.size() >= 4) {
        // x, cover, {cid}, {vid}.html
        cid = segments.at(2);
        vid = segments.at(3);
        if (vid.endsWith(".html")) vid.chop(5);
    }

    if (cid.isEmpty() || vid.isEmpty()) {
        return info;
    }

    // Resolve Node.js ckey using external script
    QString jsPath = Settings::instance().getString(QStringLiteral("qqvideo/js_path"), QCoreApplication::applicationDirPath() + "/vqq_ckey-9.1.js");

    QStringList args;
    args << jsPath << "10201" << "3.5.57" << vid << pageUrl << pageUrl;
    qDebug() << jsPath << args;
    QStringList ckeyParts = getCkey(args);
    qDebug() << ckeyParts;
    if (ckeyParts.size() < 4) {
        return info;
    }

    QString ckey = ckeyParts.at(0);
    QString tm = ckeyParts.at(1);
    QString guid = ckeyParts.at(2);
    QString flowid = ckeyParts.at(3);

    // Build vinfoparam (as x-www-form-urlencoded string)
    QString defn = server.name; // default; could be made configurable
    QString loginTokenJson = m_loginToken; // Expect compact JSON from config
    QUrlQuery vinfop;
    vinfop.addQueryItem("otype", "ojson");
    vinfop.addQueryItem("isHLS", "1");
    vinfop.addQueryItem("charge", "0");
    vinfop.addQueryItem("fhdswitch", "0");
    vinfop.addQueryItem("show1080p", "1");
    vinfop.addQueryItem("defnpayver", "7");
    vinfop.addQueryItem("sdtfrom", "v1010");
    vinfop.addQueryItem("host", "v.qq.com");
    vinfop.addQueryItem("vid", vid);
    vinfop.addQueryItem("defn", defn);
    vinfop.addQueryItem("platform", "10201");
    vinfop.addQueryItem("appVer", "3.5.57");
    vinfop.addQueryItem("refer", pageUrl);
    vinfop.addQueryItem("ehost", pageUrl);
    if (!loginTokenJson.isEmpty()) vinfop.addQueryItem("logintoken", loginTokenJson);
    vinfop.addQueryItem("encryptVer", "9.1");
    vinfop.addQueryItem("guid", guid);
    vinfop.addQueryItem("flowid", flowid);
    vinfop.addQueryItem("tm", tm);
    vinfop.addQueryItem("cKey", ckey);
    vinfop.addQueryItem("dtype", "3");
    vinfop.addQueryItem("spau", "1");
    vinfop.addQueryItem("spaudio", "68");
    vinfop.addQueryItem("spwm", "1");
    vinfop.addQueryItem("sphls", "2");
    vinfop.addQueryItem("sphttps", "1");
    vinfop.addQueryItem("clip", "4");
    vinfop.addQueryItem("spsrt", "2");
    vinfop.addQueryItem("spvvpay", "1");
    vinfop.addQueryItem("spadseg", "3");
    vinfop.addQueryItem("spav1", "15");
    vinfop.addQueryItem("hevclv", "28");
    vinfop.addQueryItem("spsfrhdr", "100");
    vinfop.addQueryItem("spvideo", "1044");
    QString vinfoparam = vinfop.query(QUrl::FullyEncoded);

    QJsonObject requestBody{
        {"buid", "vinfoad"},
        {"vinfoparam", vinfoparam}
    };

    auto bodyData = QJsonDocument(requestBody).toJson(QJsonDocument::Compact);
    // Use vd6.l.qq.com as per reference
    auto resp = client->post("https://vd6.l.qq.com/proxyhttp", bodyData, headers);
    QJsonObject root = resp.toJsonObject();
    QString vinfoStr = root.value("vinfo").toString();
    if (vinfoStr.isEmpty()) {
        return info;
    }
    QJsonParseError parseError;
    QJsonDocument vdoc = QJsonDocument::fromJson(vinfoStr.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return info;
    }
    QJsonObject vinfo = vdoc.object();
    auto viArr = vinfo.value("vl").toObject().value("vi").toArray();
    if (viArr.isEmpty()) {
        return info;
    }
    qDebug() << vinfo;
    QJsonObject vi0 = viArr.at(0).toObject();
    QJsonObject ul = vi0.value("ul").toObject();
    auto uiArr = ul.value("ui").toArray();
    QString baseUrl;
    if (!uiArr.isEmpty()) baseUrl = uiArr.at(0).toObject().value("url").toString();

    // Prefer inline m3u8 when available, otherwise fall back to base URL
    QString inlineM3u8 = ul.value("m3u8").toString();
    if (!inlineM3u8.isEmpty() && !baseUrl.isEmpty()) {
        int lastSlash = baseUrl.lastIndexOf('/');
        QString basePrefix = lastSlash >= 0 ? baseUrl.left(lastSlash) : baseUrl;
        QStringList lines = inlineM3u8.split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (!line.startsWith('#') && !line.startsWith("http")) {
                lines[i] = basePrefix + "/" + line;
            }
        }
        QString rebuilt = lines.join("\n");
        info.addHeader("Referer", pageUrl);
        info.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:144.0) Gecko/20100101 Firefox/144.0");
        info.addHeader("Accept", "*/*");
        info.addHeader("Accept-Language", "en-US,en;q=0.5");
        info.addHeader("Referer", "https://v.qq.com/");
        info.addHeader("Origin", "https://v.qq.com");
        info.addHeader("Sec-Fetch-Dest", "empty");
        info.addHeader("Sec-Fetch-Mode", "cors");
        info.addHeader("Sec-Fetch-Site", "cross-site");
        info.addHeader("Connection", "keep-alive");
        info.addHeader("DNT", "1");
        info.addHeader("Sec-GPC", "1");

        // save m3u8 to file with random name inside .tmp folder
        QString tmpDir = Settings::getTempDir();
        QString fileName = QString::number(QDateTime::currentMSecsSinceEpoch()) + ".m3u8";
        QString filePath = tmpDir + QDir::separator() + fileName;
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(rebuilt.toUtf8());
            file.close();   
        }
        info.videos.emplaceBack(QUrl::fromLocalFile(filePath));
        return info;
    }

    return info;
}



QStringList QQVideo::getCkey(QStringList &args) const {
    QString nodePath = Settings::instance().getString(QStringLiteral("node/path"), nodePath);
    if (nodePath.isEmpty()) return {};

    if (args.isEmpty()) return {};
    QString scriptPath = args.at(0);

    QProcess nodeProc;
    nodeProc.setProgram(nodePath);
    nodeProc.setArguments({scriptPath});
    nodeProc.setProcessChannelMode(QProcess::SeparateChannels);
    nodeProc.start();

    // Check if process started successfully
    if (!nodeProc.waitForStarted(5000)) {
        qWarning() << "Failed to start Node.js process:" << nodeProc.errorString();
        return {};
    }

    // Send input payload to stdin: "10201 3.5.57 vid url url\n"
    QString input = args.mid(1).join(' ');
    nodeProc.write(input.toUtf8());
    nodeProc.write("\n");
    nodeProc.closeWriteChannel();

    if (!nodeProc.waitForReadyRead(10000)) {
        qWarning() << "Node.js did not produce output in time:" << nodeProc.errorString();
    }

    QByteArray stdoutData = nodeProc.readAllStandardOutput();
    nodeProc.close();

    QList<QByteArray> parts = stdoutData.trimmed().split(' ');
    QStringList result;
    Q_FOREACH(const QByteArray &part, parts) {
        result << QString(part);
    }
    return result;
}
