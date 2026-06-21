#include "showprovider.h"
#include "app/logger.h"
#include <QRegularExpression>
#include <algorithm>
#include <numeric>

float ShowProvider::resolveTitleNumber(QString &title) const {
    if (title.startsWith(QStringLiteral("第"))) {
        static const QRegularExpression re(QStringLiteral("\\d+"));
        title = re.match(title).captured(0);
    }
    bool ok;
    float number = title.toFloat(&ok);
    if (ok)
        title = QString::number(number);
    return ok ? number : -1.0f;
}

// Merge per-server episode lists into one playlist; the link encodes every source.
int ShowProvider::parseMultiServers(ShowData &show,
                                    QVector<CSoup::Node> &serverNodes,
                                    QVector<CSoup::Node> &serverNamesNode,
                                    bool getEpisodeCountOnly) const {
    if (serverNodes.isEmpty())
        throw AppException("No servers found!", name());
    if (serverNamesNode.size() != serverNodes.size())
        throw AppException("Server name/node count mismatch!", name());

    if (getEpisodeCountOnly) {
        int maxCount = 0;
        for (auto &node : serverNodes)
            maxCount = std::max(maxCount, static_cast<int>(node.select("./li/a").size()));
        return maxCount;
    }

    // Sort servers by episode count descending so the largest sets the ordering.
    struct ServerData {
        int index;
        QVector<CSoup::Node> episodes;
    };
    QVector<ServerData> servers;
    servers.reserve(serverNodes.size());
    for (int i = 0; i < serverNodes.size(); ++i)
        servers.push_back({ i, serverNodes[i].select("./li/a") });

    std::stable_sort(servers.begin(), servers.end(), [](const ServerData &a, const ServerData &b) {
        return a.episodes.size() > b.episodes.size();
    });

    QHash<QString, QString> episodesMap;
    QList<QString> insertOrder;
    if (!servers.isEmpty()) {
        episodesMap.reserve(servers[0].episodes.size());
        insertOrder.reserve(servers[0].episodes.size());
    }

    for (const auto &server : std::as_const(servers)) {
        const QString serverName = serverNamesNode[server.index].text();
        // Iterate in reverse - episode lists are typically newest-first.
        for (int i = server.episodes.size() - 1; i >= 0; --i) {
            QString title = server.episodes[i].text();
            resolveTitleNumber(title);
            const QString link = server.episodes[i].attr("href");

            if (!episodesMap.contains(title))
                insertOrder.append(title);

            auto &links = episodesMap[title];
            if (!links.isEmpty()) links += ';';
            links += serverName + ' ' + link;
        }
    }

    for (const auto &title : std::as_const(insertOrder)) {
        bool ok;
        const float number = title.toFloat(&ok);
        show.addEpisode(0, ok ? number : -1.0f, episodesMap[title], ok ? QString() : title);
    }
    return static_cast<int>(!insertOrder.isEmpty());
}
