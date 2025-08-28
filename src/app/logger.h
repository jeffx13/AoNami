#pragma once
#include <QDebug>
#include <QFile>
#include <QHash>
#include <QVariant>
#include <QString>
#include <QAbstractListModel>
#include <QDateTime>

#define ENABLE_LOGGING

#if defined(QT_NO_DEBUG_OUTPUT)
#undef ENABLE_LOGGING
#endif

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

    QLog(Colour colour)
        : deb(qDebug().noquote().nospace())
        , m_colour(QString::fromLatin1(colorName(colour)))
    {
        deb << QString("\033[%1m[").arg(colour);
    }
    QString m_colour = "white";

    QStringList logged;

    template<typename T>
    QLog& operator<<(const T& value) {
        const QString valueStr = toStringLikeQDebug(value);
        if (fieldIndex == 0) {
            deb << centerLabel(valueStr, headerWidth) << "]";
        } else {
            deb << " " << valueStr;
        }
        ++fieldIndex;
        logged << valueStr;
        return *this;
    }

    QLog& operator<<(const char* value) { return (*this) << QString::fromUtf8(value); }
    QLog& operator<<(QStringView value) { return (*this) << value.toString(); }
    QLog& operator<<(const QByteArray& value) { return (*this) << QString::fromUtf8(value); }
    QLog& operator<<(const std::string& value) { return (*this) << QString::fromStdString(value); }


    ~QLog() {
        deb << " \033[0m";
        QMetaObject::invokeMethod(&logListModel, "addLog", Q_ARG(QStringList, logged), Q_ARG(QString, m_colour));
    }

private:
    static constexpr int headerWidth = 14;
    int fieldIndex = 0;
    QDebug deb;

    static const char* colorName(Colour colour) {
        switch (colour) {
        case Green:          return "green";
        case Red:            return "red";
        case Yellow:         return "yellow";
        case Blue:           return "blue";
        case Orange:         return "orange";
        case Magenta:        return "magenta";
        case Cyan:           return "cyan";
        case White:          return "white";
        case BrightBlue:     return "brightblue";
        case BrightMagenta:  return "brightmagenta";
        case BrightCyan:     return "brightcyan";
        case BrightWhite:    return "brightwhite";
        }
        return "white";
    }

    static QString centerLabel(const QString& text, int width) {
        const int left = (width - text.size()) / 2;
        const int right = width - (left + text.size());
        return QString(left, ' ') + text + QString(right, ' ');
    }

    template<typename T>
    static QString toStringLikeQDebug(const T& value) {
        QString s;
        QDebug dbg(&s);
        dbg.noquote().nospace() << value;
        return s;
    }
};


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
struct QLogNoop {
	template<typename T>
	QLogNoop& operator<<(const T&) { return *this; }
};

#define gLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define rLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define yLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define bLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define cLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define oLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define mLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define wLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define bbLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define bmLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define bcLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#define bwLog() for (bool _qlog_emit = false; _qlog_emit; ) QLogNoop()
#endif

// class Logger {
// private:
//     /// @brief The file object where logs are written to.
//     inline static QFile* logFile = Q_NULLPTR;

//     /// @brief Whether the logger has being initialized.
//     inline static bool isInit = false;

//     /// @brief The different type of contexts.
//     inline static QHash<QtMsgType, QString> contextNames = {
//         {QtMsgType::QtDebugMsg,		" Debug  "},
//         {QtMsgType::QtInfoMsg,		"  Info  "},
//         {QtMsgType::QtWarningMsg,	"Warning "},
//         {QtMsgType::QtCriticalMsg,	"Critical"},
//         {QtMsgType::QtFatalMsg,		" Fatal  "}
//     };

// public:
//     /// @brief Initializes the logger.
//     static void init() {
//         if (isInit) {
//             return;
//         }

//         // Create log file
//         logFile = new QFile;
//         logFile->setFileName("./MyLog.log");
//         auto opened = logFile->open(QIODevice::Append | QIODevice::Text);
//         if (!opened) {
//             delete logFile;
//             return;
//         }
//         // Redirect logs to messageOutput
//         qInstallMessageHandler(Logger::messageOutput);

//         // Clear file contents
//         logFile->resize(0);

//         Logger::isInit = true;
//     }

//     /// @brief Cleans up the logger.
//     static void clean() {
//         if (logFile != Q_NULLPTR) {
//             logFile->close();
//             delete logFile;
//         }
//     }


//     /// @brief The function which handles the logging of text.
//     static void messageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg) {

//         QString log = QObject::tr("%1 | %2 | %3 | %4 | %5 | %6\n").
//                       arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss"), Logger::contextNames.value(type)).
//                       arg(context.line).
//                       arg(QString(context.file).
//                           section('\\', -1), QString(context.function).
//                           section('(', -2, -2).		// Function name only
//                           section(' ', -1).
//                           section(':', -1), msg);

//         logFile->write(log.toLocal8Bit());
//         logFile->flush();
//         qDebug() << msg;
//     }
// };
