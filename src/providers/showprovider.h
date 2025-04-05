#pragma once
#include "utils/myexception.h"
#include "network/network.h"
#include "utils/functions.h"
#include "network/csoup.h"
#include "core/showdata.h"
#include "player/playlistitem.h"
#include "player/playinfo.h"

#include <QString>
#include <queue>


class ShowProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name CONSTANT);
    Q_PROPERTY(QString hostUrl READ hostUrl CONSTANT);
public:
    ShowProvider(QObject *parent = nullptr) : QObject(parent){};
    virtual QString name() const = 0;
    virtual QString hostUrl() const = 0;
    virtual QList<QString> getAvailableTypes() const = 0;

    virtual QList<ShowData>    search       (Client *client, const QString &query, int page, int type) = 0;
    virtual QList<ShowData>    popular      (Client *client, int page, int type) = 0;
    virtual QList<ShowData>    latest       (Client *client, int page, int type) = 0;
    virtual int                loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const = 0;
    virtual QList<VideoServer> loadServers  (Client *client, const PlaylistItem *episode) const = 0 ;
    [[nodiscard]] virtual PlayItem               extractSource(Client *client, VideoServer &server) = 0;
    inline void setPreferredServer(const QString &serverName) {
        m_preferredServer = serverName;
    }

    [[nodiscard]] QString getPreferredServer() const {
        return m_preferredServer;
    }
protected:
    QString m_preferredServer;
    int resolveTitleNumber(QString &title) const {
        if (title.startsWith("第")) {
            static QRegularExpression re("\\d+");
            title = re.match(title).captured(0);
        }
        // static auto replaceRegex = QRegularExpression("[第集话完结期]");
        float number = -1;
        bool ok;
        // title.replace(replaceRegex, "");
        float floatTitle = title.toFloat(&ok);
        if (ok){
            number = floatTitle;
            title = QString::number(number);
        }
        return number;
    }

    int parseMultiServers(ShowData &show, QVector<CSoup::Node>& serverNodes, QVector<CSoup::Node>& serverNamesNode, bool getEpisodeCountOnly) const {
        if (serverNodes.empty()) throw MyException("No servers founds!", name());
        Q_ASSERT(serverNamesNode.size() == serverNodes.size());


        if (getEpisodeCountOnly) {
            int maxEpisodeCount = 0;
            for (int i = 0; i < serverNodes.size(); i++) {
                maxEpisodeCount = std::max(maxEpisodeCount, (int)serverNodes[i].select("./li").size());
            }
            return maxEpisodeCount;
        }

        // deal with the server with the most number of episodes first
        using HelperPair = std::pair<int, std::pair<QString, QList<CSoup::Node>>>;
        auto cmp = [](const HelperPair& a,
                      const HelperPair& b) {
            return a.first < b.first; // max-heap (larger count = higher priority)
        };
        std::priority_queue<HelperPair,
                            std::vector<HelperPair>, decltype(cmp)> pq(cmp);
        for (int i = 0; i < serverNodes.size(); i++) {
            auto serverNode = serverNodes[i];
            QString serverName = serverNamesNode[i].text();
            QVector<CSoup::Node> episodeNodes = serverNode.select("./li/a");
            int episodeCount = (int)episodeNodes.size();
            pq.push(std::make_pair(episodeCount, std::make_pair(serverName, episodeNodes)));
        }

        QMap<QString, QString> episodesMap;
        QVector<QString> insertOrder;
        int serverIndex = -1;
        while (!pq.empty()) {
            auto server = pq.top();
            pq.pop();
            serverIndex++;
            auto serverName = server.second.first;
            auto episodeNodes = &server.second.second;
            for (int i = episodeNodes->size() - 1; i >= 0; i--) {
                auto episodeNode = episodeNodes->at(i);

                QString title = episodeNode.text();
                resolveTitleNumber(title);
                QString link = episodeNode.attr("href");

                if (serverIndex > 0) {
                    if (!episodesMap.contains(title)) {
                        cLog() << "Dm84" << "Inserting" << title << "at" << i;
                        insertOrder.insert(i, title);
                    }
                } else {
                    insertOrder.push_back(title);
                }

                if (!episodesMap[title].isEmpty()) episodesMap[title] += ";";
                episodesMap[title] +=  serverName + " " + link;
            }
        }

        for (const auto &title: insertOrder) {
            bool ok;
            auto number = title.toFloat(&ok);
            if (ok) {
                show.addEpisode(0, number, episodesMap[title], "");
            } else {
                show.addEpisode(0, -1, episodesMap[title], title);
            }
        }
        return true;
    }


};


