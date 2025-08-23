#pragma once
#include <QString>
#include <QObject>
#include "app/qml_singleton.h"

class UiBridge : public QObject
{
    Q_OBJECT
public:
    enum Page {
        Search = 0,
        Info = 1,
        Library = 2,
        Player = 3,
        Download = 4,
        Log = 5,
        Settings = 6
    };
    Q_ENUM(Page)

public:
    void showError(const QString &message, const QString &header = "Error")
    {
        emit errorOccurred(message, header);
    }

    void navigateTo(Page page)
    {
        emit navigateRequested(page);
    }

    static UiBridge& instance()
    {
        static UiBridge handler;
        return handler;
    }

signals:
    void errorOccurred(const QString &message, const QString &header);
    void navigateRequested(UiBridge::Page page);

private:
    UiBridge() = default;
    UiBridge(const UiBridge&) = delete;
    UiBridge& operator=(const UiBridge&) = delete;
    ~UiBridge(){}
};

DECLARE_QML_NAMED_SINGLETON(UiBridge, UiBridge);


