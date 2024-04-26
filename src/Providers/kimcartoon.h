#pragma once
#include "Providers/showprovider.h"

class Kimcartoon : public ShowProvider {
public:
    explicit Kimcartoon(QObject *parent = nullptr) : ShowProvider{parent} {};
    // std::regex sourceRegex{"sources: \\[\\{file:\"(.+?)\"\\}\\]"};
    QRegularExpression sourceRegex{"sources: \\[\\{file:\"(.+?)\"\\}\\]"};


public:
    QString name() const override { return "KIMCartoon"; }
    QString hostUrl = "https://kimcartoon.li/";

    QVector<ShowData> search(const QString &query, int page, int type) override;
    QVector<ShowData> popular(int page, int type) override;
    QVector<ShowData> latest(int page, int type) override;
    QVector<ShowData> filterSearch(const QString &url);

    bool loadDetails(ShowData &show, bool getPlaylist = true) const override;
    int getTotalEpisodes(const QString &link) const override { return 0; };
    QVector<VideoServer> loadServers(const PlaylistItem *episode) const override;;
    QList<Video> extractSource(const VideoServer &server) const override;

    QList<int> getAvailableTypes() const override { return {ShowData::ANIME}; }
private:
    QVector<ShowData> parseResults(const pugi::xpath_node_set &showNodes) {
        QVector<ShowData> shows;
        for (pugi::xpath_node_set::const_iterator it = showNodes.begin(); it != showNodes.end(); ++it) {
            auto anchor = it->node().select_node (".").node();
            QString title = QString(anchor.select_node (".//span").node().child_value()).replace ('\n'," ").trimmed();
            QString coverUrl = anchor.select_node("./img").node().attribute("src").as_string();
            if (coverUrl.startsWith ('/')) coverUrl = hostUrl + coverUrl;
            QString link = anchor.attribute("href").as_string();
            shows.emplaceBack(title, link, coverUrl, this);
        }
        return shows;
    }
};

// QString divInfo = it->node().attribute ("title").as_string();
// divInfo.remove (0,17);
// int endPosition = divInfo.indexOf("</p>");
// QString title;
// if (endPosition != -1) {
//     title = divInfo.first(endPosition);
// } else {
//     title = QString(anchor.select_node (".//span").node().child_value()).replace ('\n'," ").trimmed();
// }

