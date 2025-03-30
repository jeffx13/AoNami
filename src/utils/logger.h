#pragma once

#include <QDebug>
#include <QFile>
#include <QHash>
#include <QVariant>
#include <QString>
#include <QAbstractListModel>
#include <QDateTime>
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

class LogListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum LogRoles {
        TypeRole = Qt::UserRole,
        TimeRole,
        MessageRole
    };

    explicit LogListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    QStringList logs;
    void addLog(const QString &type, const QString &message) {
        beginInsertRows(QModelIndex(), logs.size(), logs.size());
        logs << QString("[%1] %2： %3").arg(QDateTime::currentDateTime().toString("hh:mm:ss"), type, message);
        endInsertRows();
    }
    void clearLogs() {
        beginResetModel();
        logs.clear();
        endResetModel();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return logs.size();
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid() || index.row() >= logs.size()) {
            return QVariant();
        }
        return logs.at(index.row());
    }
    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles[TimeRole] = "time";
        roles[TypeRole] = "type";
        roles[MessageRole] = "message";
        return roles;
    }
}
;



// Define ENABLE_LOGGING to enable logging; comment it out to disable logging
#define ENABLE_LOGGING

#ifdef ENABLE_LOGGING
// Logging enabled: Define QLog class and qLog macro
class QLog {
public:
    enum Colour { Green = 32, Red = 31, Yellow = 93, Blue = 34, Orange = 33, Magenta = 35, Cyan = 36 };
    inline static LogListModel logListModel{};


    QLog(Colour colour): deb(qDebug().noquote().nospace())
    {
        deb << QString("\033[%1m[").arg(colour);
    }

    QStringList logged;

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
        logged << valueStr;
        return *this;
    }

    QLog& operator<<(const std::string& valueStr) {
        auto str = QString::fromStdString(valueStr);

        if (count == 0) {
            int targetLength = 14;
            QString centered = str.leftJustified(targetLength / 2 + valueStr.length() / 2, ' ')
                                   .rightJustified(targetLength, ' ');
            deb << centered << "]:";
        } else {

            deb << " " << str;
        }
        count++;
        logged << str;
        return *this;
    }


    ~QLog() {
        deb << " \033[0m";
        auto type = logged[0];
        logged.pop_front();
        logListModel.addLog(type, logged.join(" "));
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
