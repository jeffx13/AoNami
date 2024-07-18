#pragma once
#include "Providers/showprovider.h"
#include "network/csoup.h"
class Kimcartoon : public ShowProvider {
public:
    explicit Kimcartoon(QObject *parent = nullptr) : ShowProvider{parent} {};
    // std::regex sourceRegex{"sources: \\[\\{file:\"(.+?)\"\\}\\]"};
    QRegularExpression sourceRegex{"sources: \\[\\{file:\"(.+?)\"\\}\\]"};


public:
    QString name() const override { return "KIMCartoon"; }
    QString baseUrl = "https://kimcartoon.li/";

    QVector<ShowData> search(const QString &query, int page, int type) override;
    QVector<ShowData> popular(int page, int type) override;
    QVector<ShowData> latest(int page, int type) override;
    QVector<ShowData> filterSearch(const QString &url);

    bool loadDetails(ShowData &show, bool getPlaylist = true) const override;
    int getTotalEpisodes(const QString &link) const override { return 0; };
    QVector<VideoServer> loadServers(const PlaylistItem *episode) const override;;
    PlayInfo extractSource(const VideoServer &server) const override;

    QList<int> getAvailableTypes() const override { return {ShowData::ANIME}; }
private:
    QVector<ShowData> parseResults(const QVector<CSoup::Node> &showNodes) {
        QVector<ShowData> shows;
        for (const auto &node:showNodes) {
            auto anchor = node.selectFirst(".");
            QString title = anchor.selectFirst (".//span").text().replace ('\n'," ").trimmed();
            QString coverUrl = anchor.selectFirst("./img").attr("src");
            if (coverUrl.startsWith ('/')) coverUrl = baseUrl + coverUrl;
            QString link = anchor.attr("href");
            shows.emplaceBack(title, link, coverUrl, this, "", ShowData::ANIME);
        }
        return shows;
    }
};

// QString divInfo = it->node().attrute ("title").as_string();
// divInfo.remove (0,17);
// int endPosition = divInfo.indexOf("</p>");
// QString title;
// if (endPosition != -1) {
//     title = divInfo.first(endPosition);
// } else {
//     title = QString(anchor.select_node (".//span").node().child_value()).replace ('\n'," ").trimmed();
// }

