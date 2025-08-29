#pragma once
#include <QException>
#include "app/logger.h"
#include "ui/uibridge.h"

class AppException : public QException
{
public:
    AppException(const QString &message, const QString &header = "Error") : m_message(message), m_header(header){}
    void raise() const override { throw *this; }
    QException* clone() const override { return new AppException(*this); }

private:
    QString m_message;
    QString m_header;

public:
    const char *what() const noexcept override{
        return m_message.toLocal8Bit().data();
    };
    void show() const {
        UiBridge::instance().showError(m_message, QString("%1 Error").arg(m_header));
    }
    void print() const {
        oLog() << m_header << m_message;
    }
};


