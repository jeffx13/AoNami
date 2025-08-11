#pragma once
#include "base/player/playinfo.h"
#include <QString>
#include <QVector>
#include <QJsonArray>


#include <QJSEngine>

class Vidsrcextractor
{
private:
    QJsonArray keys;
    QString keysJsonUrl = "https://raw.githubusercontent.com/Ciarands/vidsrc-keys/main/keys.json";
public:
    Vidsrcextractor();

    QVector<Video> videosFromUrl(QString embedLink, QString hosterName, QString type = "", QVector<Track> subtitleList = QVector<Track>());

    QString getApiUrl(const QString &embedLink , const QJsonArray &keyList) const;


    QString encodeID(const QString &videoID, const QJsonArray &keyList) const;


    QString callFromFuToken(const QString &host, const QString &data, const QString &embedLink) const;

    QString base64Encode(const QByteArray& input) const;

};

