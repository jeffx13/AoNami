#ifndef EPISODE_H
#define EPISODE_H
#include <QDebug>
#include <QObject>
#include <QString>

#include <QVariant>
#include <QMap>
#include <QVector>
#include <iostream>

struct Episode
{
    Q_GADGET
    Q_PROPERTY(QString title READ getTitle);
    QString getTitle() const {return title;}
    Q_PROPERTY(int number READ getNumber);
    int getNumber() const {return number;}
public:
    Episode();
    int number;
    std::string link;
    QString title = "";
    //    QString thumbnail = "";
    QString description = "";
    bool isFiller = false;
    bool hasDub = false;
    std::string dubLink;

};
struct VideoServer{
    QString name;
    std::string link;
    QMap<QString,QString> headers;
    QString source;
#
};

#endif // EPISODE_H
