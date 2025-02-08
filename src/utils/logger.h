#pragma once

#include <QDebug>
#include <QFile>
#include <QHash>
#include <QVariant>
#include <QString>
class Logger {
private:
    /// @brief The file object where logs are written to.
    static QFile* logFile;

    /// @brief Whether the logger has being initialized.
    static bool isInit;

    /// @brief The different type of contexts.
    static QHash<QtMsgType, QString> contextNames;

public:
    /// @brief Initializes the logger.
    static void init();

    /// @brief Cleans up the logger.
    static void clean();

    /// @brief The function which handles the logging of text.
    static void messageOutput(QtMsgType type, const QMessageLogContext& context,
                              const QString& msg);
};




// Define ENABLE_LOGGING to enable logging; comment it out to disable logging
#define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
// Logging enabled: Define QLog class and qLog macro
class QLog {
public:
    enum Colour { Green = 32, Red = 31, Yellow = 93, Blue = 34, Orange = 33, Magenta = 35, Cyan = 36 };


    QLog(Colour colour): deb(qDebug().noquote().nospace())
    {
        deb << QString("\033[%1m[").arg(colour);
    }

    template<typename T>
    QLog& operator<<(const T& value) {
        QString valueStr = QVariant(value).toString();  // Convert using QVariant
        if (count == 0) {
            int targetLength = 14;
            QString centered = valueStr.leftJustified(targetLength / 2 + valueStr.length() / 2, ' ')
                                   .rightJustified(targetLength, ' ');
            deb << centered << "]:";
        } else {
            deb << " " << valueStr;
        }
        count++;
        return *this;
    }

    QLog& operator<<(const std::string& valueStr) {
        if (count == 0) {
            int targetLength = 14;
            QString centered = QString::fromStdString(valueStr).leftJustified(targetLength / 2 + valueStr.length() / 2, ' ')
                                   .rightJustified(targetLength, ' ');
            deb << centered << "]:";
        } else {
            deb << " " << valueStr;
        }
        count++;
        return *this;
    }


    ~QLog() {
        deb << " \033[0m";
    }

private:
    int count = 0;
    QDebug deb;
};

// Define qLog macro
#define gLog() QLog(QLog::Green)
#define rLog() QLog(QLog::Red)
#define yLog() QLog(QLog::Yellow)
#define bLog() QLog(QLog::Blue)
#define cLog() QLog(QLog::Cyan)
#define oLog() QLog(QLog::Orange)
#define mLog() QLog(QLog::Magenta)

#else
// Logging disabled: Define a no-op qLog macro
#define gLog() if (false) gLog()
#define cLog() if (false) cLog()
#define oLog() if (false) oLog()
#define rLog() if (false) rLog()
#define yLog() if (false) yLog()
#define bLog() if (false) bLog()
#define mLog() if (false) mLog()

#endif
