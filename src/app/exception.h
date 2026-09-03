#pragma once
#include <QException>
#include "app/logger.h"
#include "ui/appshell.h"

class AppException : public QException
{
public:
    AppException(const QString &message, const QString &header = "Error")
        : m_message(message), m_header(header), m_whatBuffer(message.toUtf8()) {}

    void raise() const override { throw *this; }
    QException* clone() const override { return new AppException(*this); }

    const char *what() const noexcept override {
        return m_whatBuffer.constData();
    }

    void report() const {
        AppShell::instance().reportError(m_message, QString("%1 Error").arg(m_header));
    }

    void log() const {
        logWarn() << m_header << m_message;
    }

private:
    QString m_message;
    QString m_header;
    QByteArray m_whatBuffer;
};
