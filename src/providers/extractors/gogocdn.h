#pragma once

#include <QCryptographicHash>
#include <QByteArray>
#include <string>

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>

#include <cryptopp/cryptlib.h>
#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>

// #include <regex>
#include "utils/functions.h"
#include "network/csoup.h"

class GogoCDN
{
private:

    struct Keys {
        std::string key;
        std::string secondKey;
        std::string iv;
    };
    Keys keysAndIv
        {
            "37911490979715163134003223491201",
            "54674138327930866480207815084989",
            "3134003223491201"
        };
public:
    GogoCDN(){};
    QString extract(const QString &link)
    {
        qDebug() << link;
        if (link.contains ("abpl1245"))
        {
            auto document = CSoup::connect(link);
            if (!document) return "";
            qDebug() << "success";
            QString episodeDataValue = document.selectFirst("//script[@data-name='episode']").attr("data-value");

            QString decrypted = QString(decrypt(episodeDataValue, keysAndIv.key, keysAndIv.iv).data()).remove('\t');
            auto id = Functions::findBetween(decrypted, "", "&");
            auto end = Functions::substringAfter(decrypted, id);

            auto encryptedId = QString::fromStdString (encrypt(id, keysAndIv.key, keysAndIv.iv));
            QString encryptedUrl = "https://" + Functions::getHostFromUrl(link)
                                   + "/encrypt-ajax.php?id=" + encryptedId + end + "&alias=" + id;
            QString encrypted = Client::post(encryptedUrl, {{"X-Requested-With", "XMLHttpRequest"}}).body;

            QString dataEncrypted = Functions::findBetween(encrypted, "{\"data\":\"", "\"}");
            auto jumbledJsonString = QString::fromStdString (decrypt(dataEncrypted, keysAndIv.secondKey, keysAndIv.iv));
            jumbledJsonString.replace("o\"<P{#meme=\"\"","{\"e\":[{\"file\":\"");

            QJsonObject sourceJson = QJsonDocument::fromJson(jumbledJsonString.toUtf8()).object();
            auto source = sourceJson["source"].toArray()[0].toObject()["file"].toString();
            // auto source_bk = sourceJson["source_bk"].toArray()[0].toObject()["file"].toString();
            return source;
        }
        return "";
    }

    std::string encrypt(const QString& str, const std::string& key, const std::string& iv) {
        using namespace CryptoPP;
        AES::Encryption aesKey(reinterpret_cast<const unsigned char*>(key.data()), key.size());
        CBC_Mode_ExternalCipher::Encryption cbcEncryptor(aesKey, reinterpret_cast<const unsigned char*>(iv.data()));

        std::string plaintext = str.toStdString();
        std::string ciphertext;
        StringSource(plaintext, true,
                               new StreamTransformationFilter(cbcEncryptor,
                                                                        new Base64Encoder(
                                                                            new StringSink(ciphertext), false
                                                                            ), BlockPaddingSchemeDef::PKCS_PADDING
                                                                        )
                               );
        return ciphertext;
    }

    std::string decrypt(const QString& str, const std::string& key, const std::string& iv) {
        using namespace CryptoPP;

        std::string plaintext;
        std::string ciphertext = str.toStdString();
        try {
            CBC_Mode<AES>::Decryption decryption(reinterpret_cast<const byte*>(key.data()), key.size(),
                                                                     reinterpret_cast<const byte*>(iv.data()));
            StringSource(ciphertext, true,
                                   new Base64Decoder(
                                       new StreamTransformationFilter(decryption,
                                                                                new StringSink(plaintext))));
        }
        catch (const Exception& e){
            qWarning() << e.what();
            return "";
        }
        return plaintext;
    }


public:

};



