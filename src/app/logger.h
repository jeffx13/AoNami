#pragma once
#include <QDebug>
#include <QFile>
#include <QHash>
#include <QVariant>
#include <QString>
#include <QAbstractListModel>
#include <QDateTime>
#include <QMutex>
#include <QCoreApplication>
#include <qqmlintegration.h>

#define ENABLE_LOGGING

#if defined(QT_NO_DEBUG_OUTPUT)
#undef ENABLE_LOGGING
#endif

class LogListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ANONYMOUS
public:
    enum LogRoles { TypeRole = Qt::UserRole, TimeRole, MessageRole, ColourRole };

    explicit LogListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    Q_INVOKABLE void addLog(QStringList log, QString colour) {
        beginInsertRows(QModelIndex(), m_logs.size(), m_logs.size());
        log.insert(0, QDateTime::currentDateTime().toString("hh:mm:ss"));
        log.append(colour);
        m_logs.append(log);
        endInsertRows();
    }

    Q_INVOKABLE void clear() {
        beginResetModel();
        m_logs.clear();
        endResetModel();
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override { return m_logs.size(); }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!index.isValid()) return {};
        const auto &log = m_logs.at(index.row());

        switch (role) {
        case TimeRole:    return log[0];
        case TypeRole:    return log[1];
        case ColourRole:  return log.last();
        case MessageRole: {
            if (log[1].startsWith("GET") || log[1].startsWith("POST"))
                return QString("<a href='%1'>%1</a>").arg(log[2]);
            // Join all parts between type and colour
            return QStringList(log.mid(2, log.size() - 3)).join(" ");
        }
        default: return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override {
        return {
                {TimeRole, "time"}, {TypeRole, "type"},
                {MessageRole, "message"}, {ColourRole, "colour"},
                };
    }

private:
    QList<QStringList> m_logs;
};

#ifdef ENABLE_LOGGING

class QLog {
public:
    enum Colour {
        Red = 31, Green = 32, Orange = 33, Blue = 34, Magenta = 35,
        Cyan = 36, White = 37, Yellow = 93, BrightBlue = 94,
        BrightMagenta = 95, BrightCyan = 96, BrightWhite = 97
    };

    inline static LogListModel logListModel{};

    QLog(Colour colour)
        : m_deb(qDebug().noquote().nospace())
        , m_colour(QString::fromLatin1(colourName(colour)))
    {
        m_deb << QString("\033[%1m[").arg(colour);
    }

    ~QLog() {
        m_deb << " \033[0m";
        QMetaObject::invokeMethod(&logListModel, "addLog",
                                  Q_ARG(QStringList, m_logged),
                                  Q_ARG(QString, m_colour));
        writeToFile(m_logged);
    }

    // Mirror every line to <exe-dir>/aonami.log (truncated each run); thread-safe.
    static void writeToFile(const QStringList &fields) {
        static QMutex mtx;
        static QFile file;
        QMutexLocker lk(&mtx);
        if (!file.isOpen()) {
            file.setFileName(QCoreApplication::applicationDirPath() + "/aonami.log");
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
                return;
        }
        file.write((QDateTime::currentDateTime().toString("hh:mm:ss") + " " +
                    fields.join(" ") + "\n").toUtf8());
        file.flush();
    }

    template<typename T>
    QLog &operator<<(const T &value) {
        const QString str = toStr(value);
        if (m_fieldIndex == 0)
            m_deb << centerLabel(str, kHeaderWidth) << "]";
        else
            m_deb << " " << str;
        ++m_fieldIndex;
        m_logged << str;
        return *this;
    }

    QLog &operator<<(const char *v)        { return *this << QString::fromUtf8(v); }
    QLog &operator<<(QStringView v)        { return *this << v.toString(); }
    QLog &operator<<(const QByteArray &v)  { return *this << QString::fromUtf8(v); }
    QLog &operator<<(const std::string &v) { return *this << QString::fromStdString(v); }

private:
    static constexpr int kHeaderWidth = 14;
    int m_fieldIndex = 0;
    QDebug m_deb;
    QString m_colour;
    QStringList m_logged;

    static const char *colourName(Colour c) {
        switch (c) {
        case Green:         return "green";
        case Red:           return "red";
        case Yellow:        return "yellow";
        case Blue:          return "blue";
        case Orange:        return "orange";
        case Magenta:       return "magenta";
        case Cyan:          return "cyan";
        case White:         return "white";
        case BrightBlue:    return "brightblue";
        case BrightMagenta: return "brightmagenta";
        case BrightCyan:    return "brightcyan";
        case BrightWhite:   return "brightwhite";
        }
        return "white";
    }

    static QString centerLabel(const QString &text, int width) {
        int left = (width - text.size()) / 2;
        int right = width - left - text.size();
        return QString(left, ' ') + text + QString(right, ' ');
    }

    template<typename T>
    static QString toStr(const T &value) {
        QString s;
        QDebug(&s).noquote().nospace() << value;
        return s;
    }
};

#define gLog()  QLog(QLog::Green)
#define rLog()  QLog(QLog::Red)
#define yLog()  QLog(QLog::Yellow)
#define bLog()  QLog(QLog::Blue)
#define cLog()  QLog(QLog::Cyan)
#define oLog()  QLog(QLog::Orange)
#define mLog()  QLog(QLog::Magenta)
#define wLog()  QLog(QLog::White)
#define bbLog() QLog(QLog::BrightBlue)
#define bmLog() QLog(QLog::BrightMagenta)
#define bcLog() QLog(QLog::BrightCyan)
#define bwLog() QLog(QLog::BrightWhite)

#else

// Logging disabled - all macros compile to nothing
struct QLogNoop {
    template<typename T> QLogNoop &operator<<(const T &) { return *this; }
};

// Must still provide logListModel for code that references QLog::logListModel
class QLog {
public:
    inline static LogListModel logListModel{};
};

#define gLog()  QLogNoop()
#define rLog()  QLogNoop()
#define yLog()  QLogNoop()
#define bLog()  QLogNoop()
#define cLog()  QLogNoop()
#define oLog()  QLogNoop()
#define mLog()  QLogNoop()
#define wLog()  QLogNoop()
#define bbLog() QLogNoop()
#define bmLog() QLogNoop()
#define bcLog() QLogNoop()
#define bwLog() QLogNoop()

#endif
