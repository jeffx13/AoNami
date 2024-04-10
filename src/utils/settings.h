#pragma once

#include <QtGlobal>
#include <QObject>
#include <QNetworkProxyFactory>


class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = nullptr) : QObject{parent} {};
    void setProxy(const QString& proxyString){
        QByteArray proxy = proxyString.toUtf8();
        qputenv("http_proxy", proxy);
        qputenv("https_proxy", proxy);
        QNetworkProxyFactory::setUseSystemConfiguration( false );
    }

signals:

};
