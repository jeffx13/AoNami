#ifndef GOGOCDN_H
#define GOGOCDN_H

#include "videoextractor.cpp"

#include <parsers/episode.h>

#include <cstring>
#include <iostream>
#include <string>
#include <network/client.h>
#include <QRegularExpression>
#include <cryptopp/cryptlib.h>
#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <functions.hpp>
#include <regex>

class GogoCDN: public VideoExtractor
{
public:
    GogoCDN();
    struct Keys {
        std::string key;
        std::string secondKey;
        std::string iv;
    };

    bool extract(VideoServer *server)override{
        NetworkClient client;
        auto response = client.get(server->link).document();
        if (Functions::containsSubstring (server->link,"streaming.php")){
            std::string it =response.selectFirst("//script[@data-name='episode']").attr("data-value").value ();
            std::string decrypted = decrypt (it, keysAndIv.key, keysAndIv.iv);
            decrypted.erase(std::remove(decrypted.begin(), decrypted.end(), '\t'), decrypted.end());  // Remove all occurrences of '\t'
            std::string id = Functions::findBetween(decrypted, "", "&");
            std::string end = Functions::substringAfter(decrypted,id);
            std::string encryptedId = encrypt(id, keysAndIv.key, keysAndIv.iv);
            std::string encryptedUrl = "https://" + getHostFromUrl(server->link)+ "/encrypt-ajax.php?id=" + encryptedId + end + "&alias=" + id;
            std::string encrypted = client.get(encryptedUrl, {{"X-Requested-With", "XMLHttpRequest"}}).body;
            std::string dataEncrypted = Functions::findBetween(encrypted, "{\"data\":\"", "\"}");
            std::string jumbledJson = decrypt(dataEncrypted, keysAndIv.secondKey, keysAndIv.iv);
            Functions::replaceAll(jumbledJson,R"(o"<P{#meme="")",R"({"e":[{"file":")");
            auto source = nlohmann::json::parse (jumbledJson)["source"][0]["file"].get <std::string>();
            auto source_bk = nlohmann::json::parse (jumbledJson)["source_bk"][0]["file"].get <std::string>();
            //            qDebug()<<"source"<<QString::fromStdString (source)<<"\n";
            //            qDebug()<<"source_bk"<<QString::fromStdString (source_bk)<<"\n";
            server->source=QString::fromStdString (source);
            return true;
        }
        return false;
    }

    std::string getHostFromUrl(const std::string& url) {
        std::regex regex("^(?:https?://)?(?:www\\.)?([^:/\\s]+)");
        std::smatch match;
        if (std::regex_search(url, match, regex)) {
            return match[1];
        }
        return "";
    }

    std::string encrypt(const std::string& str, const std::string& key, const std::string& iv) {
        CryptoPP::AES::Encryption aesKey(reinterpret_cast<const unsigned char*>(key.data()), key.size());
        CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryptor(aesKey, reinterpret_cast<const unsigned char*>(iv.data()));

        std::string ciphertext;
        CryptoPP::StringSource(str, true,
                               new CryptoPP::StreamTransformationFilter(cbcEncryptor,
                                                                        new CryptoPP::Base64Encoder(
                                                                            new CryptoPP::StringSink(ciphertext), false
                                                                            ), CryptoPP::BlockPaddingSchemeDef::PKCS_PADDING
                                                                        )
                               );
        return ciphertext;
    }

    std::string decrypt(const std::string& ciphertext, const std::string& key, const std::string& iv)
    {
        std::string plaintext;
        try {
            CryptoPP::CBC_Mode<CryptoPP::AES>::Decryption decryption(reinterpret_cast<const byte*>(key.data()), key.size(),
                                                                     reinterpret_cast<const byte*>(iv.data()));
            CryptoPP::StringSource(ciphertext, true,
                                   new CryptoPP::Base64Decoder(
                                       new CryptoPP::StreamTransformationFilter(decryption,
                                                                                new CryptoPP::StringSink(plaintext))));
        }
        catch (const CryptoPP::Exception& e) {
            std::cerr << e.what() << std::endl;
            return "";
        }
        return plaintext;
    }

    Keys keysAndIv {
        "37911490979715163134003223491201",
        "54674138327930866480207815084989",
        "3134003223491201"
    };
    // VideoExtractor interface
public:

};

#endif // GOGOCDN_H
