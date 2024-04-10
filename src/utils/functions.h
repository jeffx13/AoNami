#pragma once


#include <QString>
#include <QDebug>
#include <cryptopp/base64.h>
#include <cryptopp/des.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>

#include <cryptopp/hex.h>
#include <QException>
#include <QRegularExpression>

namespace Functions{
inline void httpsIfy(QString& text){
    if (text.startsWith ("//")){
        text = "https:" + text;
    }
}
QString getHostFromUrl(const QString& url);
QString findBetween(const QString &str, const QString &a, const QString &b);
QString substringAfter(const QString &str, const QString &delimiter);
QString substringBefore(const QString &str, const QString &delimiter);


// std::string reverseString(const std::string& str);
// std::string rc4(const std::string& key, const std::string& input);
// std::string rot13(std::string n);
// std::string base64Encode(const std::string& t,const std::string& baseTable="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
// std::string base64Decode(const std::string& input,const std::string& baseTable =  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
// std::vector<std::string> split(const std::string& str, char delimiter);
// std::vector<std::string> split(const std::string& str, const std::string& delimiter);
// std::string MD5(const std::string& str);

};



