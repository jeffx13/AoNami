// #pragma once
// #include <QDebug>
// #include "showprovider.h"

// #include <QDateTime>
// #include <QProcess>

// #include "utils/wasmengine.h"
// #include <QUrl>
// // #include <BigInt.hpp>
// class BigInt;


// class Cineby : public ShowProvider
// {
// public:
//     explicit Cineby(QObject *parent = nullptr) : ShowProvider{parent} {
//         this->filterChars = "cfhistuCFHISTU";
//         this->alphabet = Functions::filter(alphabet, this->filterChars);
//         size_t o = std::ceil(static_cast<double>(this->alphabet.length()) / 3.5);
//         size_t h = o - this->filterChars.length();
//         this->filterChars += this->alphabet.substr(0, h);
//         size_t b = std::ceil(static_cast<double>(this->alphabet.length()) / 12);
//         this->guards = this->alphabet.substr(0, b);
//         this->alphabet = this->alphabet.substr(b);
//     }
//     QString            name() const override { return "Cineby"; }
//     QString            hostUrl ()const override { return "https://www.cineby.app/"; }
//     QList<int>         getAvailableTypes()const override { return {ShowData::TVSERIES, ShowData::MOVIE}; }
//     QList<ShowData>    search           (Client *client, const QString &query, int page, int type) override;
//     QList<ShowData>    popular          (Client *client, int page, int type) override;
//     QList<ShowData>    latest           (Client *client, int page, int type) override;
//     int                loadDetails      (Client *client, ShowData &show, bool loadInfo, bool getPlaylist, bool getEpisodeCount) const override;
//     QList<VideoServer> loadServers      (Client *client, const PlaylistItem* episode) const override;
//     PlayInfo           extractSource    (Client *client, VideoServer& server) override;


// private:
//     WasmEngine& getEngine(Client *client);


//     std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
//     std::string filterChars;
//     std::string guards;

//     std::string encode(BigInt input)  const;
//     std::string shuffle(std::string i, const std::string& e) const;

//     QMap<QString, QString> m_headers = {
//         {"Accept", "*/*"},
//         {"Accept-Language", "en-GB,en;q=0.9,en-US;q=0.8"},
//         {"Cache-Control", "no-cache"},
//         {"Connection", "keep-alive"},
//         {"Referer", "https://www.cineby.app/"},
//         {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 Edg/131.0.0.0"},

//     };
//     QList<ShowData> getSection(Client *client, int section);


//     std::optional<wasmtime::Memory> memory;
//     std::optional<wasmtime::Func> decrypt_func;
//     std::optional<wasmtime::Func> new_func;

//     std::string hash(const std::string& input) const;

//     std::string decrypt(WasmEngine& engine, const std::string &input, int tmdbId) const;

// };



