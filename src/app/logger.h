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

// Define ENABLE_LOGGING to enable logging; comment it out to disable logging
#define ENABLE_LOGGING

class LogListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum LogRoles {
        TypeRole = Qt::UserRole,
        TimeRole,
        MessageRole,
        ColourRole
    };

    explicit LogListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    QList<QStringList> logs;

    Q_INVOKABLE void addLog(QStringList log, QString colour) {
        beginInsertRows(QModelIndex(), logs.size(), logs.size());
        log.insert(0, QDateTime::currentDateTime().toString("hh:mm:ss"));
        log.append(colour);
        logs.append(log);
        endInsertRows();
    }

    Q_INVOKABLE void clear() {
        beginResetModel();
        logs.clear();
        endResetModel();
        emit layoutChanged();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        Q_UNUSED(parent);
        return logs.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid()) {
            return QVariant();
        }
        auto log = logs.at(index.row());

        switch (role) {
        case TimeRole:
            return log[0];
        case TypeRole:
            return log[1];
        case MessageRole:
        {
            if (log[1].startsWith("GET") || log[1].startsWith("POST")) {
                return QString("<a href='%1'>%1</a></html>").arg(log[2]);
            }

            // Join the rest of the log messages with spaces except last
            QStringList messageParts = log.mid(2);
            messageParts.removeLast(); // Remove the last part which is colour
            return QString("%1").arg(messageParts.join(" "));

        }
        case ColourRole:
            return log.last();
        default:
            return QVariant();
        }

    }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles[TimeRole] = "time";
        roles[TypeRole] = "type";
        roles[MessageRole] = "message";
        roles[ColourRole] = "colour";
        return roles;
    }
};




#ifdef ENABLE_LOGGING
// Logging enabled: Define QLog class and qLog macro
class QLog {
public:
    enum Colour {
        Red           = 31,
        Green         = 32,
        Orange        = 33,
        Blue          = 34,
        Magenta       = 35,
        Cyan          = 36,
        White         = 37,
        Yellow        = 93,
        BrightBlue    = 94,
        BrightMagenta = 95,
        BrightCyan    = 96,
        BrightWhite   = 97
    };
    inline static LogListModel logListModel{};


    QLog(Colour colour): deb(qDebug().noquote().nospace())
    {
        deb << QString("\033[%1m[").arg(colour);
        switch (colour) {
        case Green:
            m_colour = "green";
            break;
        case Red:
            m_colour = "red";
            break;
        case Yellow:
            m_colour = "yellow";
            break;
        case Blue:
            m_colour = "blue";
            break;
        case Orange:
            m_colour = "orange";
            break;
        case Magenta:
            m_colour = "magenta";
            break;
        case Cyan:
            m_colour = "cyan";
            break;
        case White:
            m_colour = "white";
            break;
        case BrightBlue:
            m_colour = "brightblue";
            break;
        case BrightMagenta:
            m_colour = "brightmagenta";
            break;
        case BrightCyan:
            m_colour = "brightcyan";
            break;
        case BrightWhite:
            m_colour = "brightwhite";
            break;
        }
    }
    QString m_colour = "white";

    QStringList logged;

    template<typename T>
    QLog& operator<<(const T& value) {
        QString valueStr = QVariant(value).toString();  // Convert using QVariant
        if (count == 0) {
            int targetLength = 14;
            QString centered = valueStr.leftJustified(targetLength / 2 + valueStr.length() / 2, ' ')
                                   .rightJustified(targetLength, ' ');
            deb << centered << "]";
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
        QMetaObject::invokeMethod(&logListModel, "addLog", Q_ARG(QStringList, logged), Q_ARG(QString, m_colour));
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
#define wLog() QLog(QLog::White)
#define bbLog() QLog(QLog::BrightBlue)
#define bmLog() QLog(QLog::BrightMagenta)
#define bcLog() QLog(QLog::BrightCyan)
#define bwLog() QLog(QLog::BrightWhite)


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
