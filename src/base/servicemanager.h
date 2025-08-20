#pragma once
#include <QObject>

class ServiceManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
public:
    ServiceManager(QObject *parent = nullptr) : QObject(parent) {}

    Q_SIGNAL void isLoadingChanged(void);
    bool isLoading() { return m_isLoading; }
    //Q_INVOKABLE void cancel();
protected:
    std::atomic<bool> m_isCancelled = false;    
    void setIsLoading(bool b) { m_isLoading = b; emit isLoadingChanged(); }
private:
    bool m_isLoading = false;
};

