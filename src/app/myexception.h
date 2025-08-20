#pragma once
#include <QException>
#include "app/logger.h"
#include "gui/errordisplayer.h"
class MyException : public QException
{
public:
    MyException(const QString &message, const QString &header = "Error") : m_message(message), m_header(header){}
    void raise() const override { throw *this; }
    QException* clone() const override { return new MyException(*this); }

private:
    QString m_message;
    QString m_header;


public:
    const char *what() const noexcept override{
        return m_message.toLocal8Bit().data();
    };
    void show() const {
        ErrorDisplayer::instance().show(m_message, QString("%1 Error").arg(m_header));
    }
    void print() const {
        oLog() << m_header << m_message;
    }
};

