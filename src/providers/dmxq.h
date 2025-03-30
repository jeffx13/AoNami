#pragma once

#include "providers/showprovider.h"
#include <cryptopp/hmac.h>
#include <cryptopp/md5.h>
#include <cryptopp/hex.h>
#include <cryptopp/rsa.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h>
#include <cryptopp/secblock.h>


class Dmxq : public ShowProvider
{
public:
    explicit Dmxq(QObject *parent = nullptr) : ShowProvider(parent) {};
    QString name() const override { return "大米星球"; }
    QString hostUrl() const override { return "https://www.dmph5.wiki/"; }
    QList<QString> getAvailableTypes() const override {

        return {"动漫", "电影", "电视剧", "综艺"};
    };

    QList<ShowData>    search       (Client *client, const QString &query, int page, int type) override;
    QList<ShowData>    popular      (Client *client, int page, int type) override;
    QList<ShowData>    latest       (Client *client, int page, int type) override;
    int                loadDetails  (Client *client, ShowData &show, bool getEpisodeCountOnly, bool fetchPlaylist) const override;
    QList<VideoServer> loadServers  (Client *client, const PlaylistItem* episode) const override;
    PlayInfo           extractSource(Client *client, VideoServer &server) override;
private:
    QList<ShowData>    filterSearch (Client *client, int type, const QString &sortBy, int page);
    QList<int> types = {
        4, // 动漫
        1, // 电影
        2, // 电视剧
        3  // 综艺
    };

    QString encryptHMAC2String(const QString& data, const QString& key = "635a580fcb5dc6e60caa39c31a7bde48") const {
        using namespace CryptoPP;
        std::string result;
        std::string input = data.toStdString();
        std::string keyStr = key.toStdString();

        try {
            HMAC<Weak::MD5> hmac((byte*)keyStr.data(), keyStr.size());
            std::string mac;
            StringSource ss(input, true, new HashFilter(hmac, new StringSink(mac)));

            // Convert to hex
            StringSource ss2(mac, true, new HexEncoder(new StringSink(result), false));
        } catch (const Exception& e) {
            qWarning("Crypto++ Error: %s", e.what());
            return {};
        }

        return QString::fromStdString(result);
    }

    QString newEncode(const QString& plaintext) const {
        using namespace CryptoPP;

        const std::string base64Key =
            "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA02F/kPg5A2NX4qZ5JSns"
            "+bjhVMCC6JbTiTKpbgNgiXU+Kkorg6Dj76gS68gB8llhbUKCXjIdygnHPrxVHWfzm"
            "zisq9P9awmXBkCk74Skglx2LKHa/mNz9ivg6YzQ5pQFUEWS0DfomGBXVtqvBlOXMC"
            "Rxp69oWaMsnfjnBV+0J7vHbXzUIkqBLdXSNfM9Ag5qdRDrJC3CqB65EJ3ARWVzZTT"
            "cXSdMW9i3qzEZPawPNPe5yPYbMZIoXLcrqvEZnRK1oak67/ihf7iwPJqdc+68ZYEm"
            "mdqwunOvRdjq89fQMVelmqcRD9RYe08v+xDxG9Co9z7hcXGTsUquMxkh29uNawID"
            "AQAB";

        std::string decodedKey;
        StringSource base64Decode(base64Key, true, new Base64Decoder(new StringSink(decodedKey)));

        try {
            // Load public key from raw DER
            CryptoPP::RSA::PublicKey publicKey;
            publicKey.Load(StringStore(decodedKey).Ref());

            // Encrypt plaintext
            AutoSeededRandomPool rng;
            std::string cipher;
            RSAES_PKCS1v15_Encryptor encryptor(publicKey);
            StringSource encryptorSrc(plaintext.toStdString(), true,
                                      new PK_EncryptorFilter(rng, encryptor, new StringSink(cipher)));

            // Base64 URL-safe encode the result
            std::string encoded;
            Base64Encoder encoder(new StringSink(encoded), false); // No line breaks
            encoder.Put((byte*)cipher.data(), cipher.size());
            encoder.MessageEnd();

            // Convert to URL-safe: replace '+' → '-', '/' → '_', and strip '='
            for (char& ch : encoded) {
                if (ch == '+') ch = '-';
                else if (ch == '/') ch = '_';
            }
            encoded.erase(std::remove(encoded.begin(), encoded.end(), '='), encoded.end());

            return QString::fromStdString(encoded);

        } catch (const Exception& e) {
            qWarning("RSA Encrypt Error: %s", e.what());
            return {};
        }
    }

    QJsonObject invokeAPI(Client *client, const QString &path, const QString& params) const {
        auto pack = newEncode(params);
        auto signature = encryptHMAC2String(pack, "635a580fcb5dc6e60caa39c31a7bde48");
        QString url = QString("https://dsvhona1mi.37ymztjy.com/api/v1/movie/%1?pack=%2&signature=%3").arg(path, pack, signature);

        QJsonObject response = client->get(url).toJsonObject();
        auto msg = response["msg"].toString();
        if (!msg.contains("成功"))
            throw MyException(response["msg"].toString(), name());
        return response;
    }

};
