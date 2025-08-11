#pragma once
#include <QString>
#include <QObject>
#include "app/qml_singleton.h"
class ErrorDisplayer : public QObject
{
    Q_OBJECT
public:
    void show(const QString& errorMessage, const QString& errorHeader = "Error"){
        emit showWarning (errorMessage, errorHeader);
    };
    static ErrorDisplayer& instance(){
        static ErrorDisplayer handler;
        return handler;
    }
signals:
    void showWarning(const QString &message, const QString &header);

private:
    ErrorDisplayer() = default;
    ErrorDisplayer(const ErrorDisplayer&) = delete; // Disable copy constructor.
    ErrorDisplayer& operator=(const ErrorDisplayer&) = delete; // Disable copy assignment.
    ~ErrorDisplayer(){} // Private destructor to prevent external deletion.
};

DECLARE_QML_SINGLETON(ErrorDisplayer);
