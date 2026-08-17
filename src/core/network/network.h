#pragma once
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include "core/network/csoup.h"
#include "core/network/canceltoken.h"

class Client {
public:
    Client(CancelToken cancel = {}, bool verbose = true)
        : m_cancel(std::move(cancel)), m_verbose(verbose) {}

    Client(const Client &other) = default;
    Client &operator=(const Client &other) = default;

    bool isCancelled() const { return m_cancel.isCancelled(); }

    // A copy of this client that also aborts when `secondary` fires (race losers).
    Client withCancel(const CancelToken &secondary) const {
        Client c = *this;
        c.m_cancel = m_cancel.composeWith(secondary);
        return c;
    }

    // Off for endpoints that must not recurse (the solver's own calls) or where a
    // 403 is a real answer.
    Client &setBypassEnabled(bool enabled) { m_bypass = enabled; return *this; }

    // For callers that log their own line - miruro's urls are 300 chars of base64.
    Client &setVerbose(bool verbose) { m_verbose = verbose; return *this; }


    struct Response {
        int code = -1;
        QMap<QString, QString> headers;
        QString body;
        QByteArray bytes;   // getBytes() only; body stays empty there

        // HTTP header names are case-insensitive; HTTP/2 lowercases them.
        QString header(const QString &name) const {
            for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
                if (it.key().compare(name, Qt::CaseInsensitive) == 0)
                    return it.value();
            return {};
        }

        QJsonObject toJsonObject() const {
            if (body.isEmpty()) return {};
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(body.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) return {};
            return doc.object();
        }

        QJsonArray toJsonArray() const {
            if (body.isEmpty()) return {};
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(body.toUtf8(), &error);
            if (error.error != QJsonParseError::NoError) return {};
            return doc.array();
        }

        CSoup toSoup() const { return CSoup::parse(body); }
    };

    Response get(const QString &url,
                 const QMap<QString, QString> &headers = {},
                 const QMap<QString, QString> &params = {});

    // For binary bodies, which QString::fromUtf8 would fill with U+FFFD.
    Response getBytes(const QString &url,
                      const QMap<QString, QString> &headers = {},
                      const QMap<QString, QString> &params = {});

    // Form-encoded POST
    Response post(const QString &url,
                  const QMap<QString, QString> &data,
                  const QMap<QString, QString> &headers = {});

    // Raw body POST (for JSON, etc.)
    Response post(const QString &url,
                  const QByteArray &data,
                  const QMap<QString, QString> &headers = {}) {
        return request(POST, url, headers, data);
    }

    Response head(const QString &url,
                  const QMap<QString, QString> &headers = {}) {
        return request(HEAD, url, headers);
    }

    // Returns true if server responds with 200 or 206 to a range request
    bool partialGet(const QString &url,
                    const QMap<QString, QString> &headers = {},
                    const QString &range = "0-0") {
        auto h = headers;
        h.insert("Range", "bytes=" + range);
        auto resp = get(url, h);
        return resp.code == 206 || resp.code == 200;
    }

private:
    enum RequestType { GET, POST, HEAD };
    Response request(int type, const QString &url,
                     const QMap<QString, QString> &headers,
                     const QByteArray &postData = {},
                     bool binary = false);

    CancelToken m_cancel;
    bool        m_verbose = true;
    bool        m_bypass = true;
};
