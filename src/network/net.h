// #pragma once
// #include <QDebug>
// #include <QJsonDocument>
// #include <QJsonObject>
// #include <QNetworkReply>
// #include <QNetworkRequest>
// #include <QJsonArray>
// #include <QNetworkAccessManager>
// #include <QFuture>
// #include <QPromise>
// #include <QObject>
// #include <QMap>
// #include <QUrlQuery>

// class QClient : public QObject
// {
//     Q_OBJECT

// private:
//     enum RequestType {
//         GET,
//         POST,
//         HEAD,
//         PUT,
//         DELETE_,
//         CONNECT,
//         OPTIONS
//     };

// public:
//     struct Response {
//     private:
//         QNetworkReply *reply;
//         template <typename T>
//         QFuture<T> getAttribute(std::function<T()> getter) const {
//             QPromise<T> promise;
//             QFuture<T> future = promise.future();

//             if (reply->isFinished()) {
//                 T result = getter();
//                 promise.addResult(result);
//                 promise.finish();
//             } else {
//                 QObject::connect(reply, &QNetworkReply::finished, [getter, p = std::move(promise)]() mutable {
//                     T result = getter();
//                     p.addResult(result);
//                     p.finish();
//                 });
//             }
//             return future;
//         }
//     public:
//         explicit Response(QNetworkReply *reply) : reply(reply) {}

//         QFuture<int> code() const {
//             return getAttribute<int>([this]() {
//                 return reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
//             });
//         }

//         QFuture<QString> redirectUrl() const {
//             return getAttribute<QString>([this]() {
//                 return reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
//             });
//         }

//         QFuture<QString> headers() const {
//             return getAttribute<QString>([this]() {
//                 return reply->rawHeaderList().join("\n");
//             });
//         }

//         QFuture<QString> body() const {
//             return getAttribute<QString>([this]() {
//                 return reply->readAll();
//             });
//         }

//         QFuture<QJsonDocument> json() const {
//             return body().then([](const QString &body) -> QJsonDocument {
//                 QJsonParseError error;
//                 QJsonDocument jsonData = QJsonDocument::fromJson(body.toUtf8(), &error);
//                 if (error.error != QJsonParseError::NoError) {
//                     qWarning() << "JSON parsing error:" << error.errorString();
//                     return QJsonDocument{};
//                 }
//                 return jsonData;
//             });
//         }

//         QFuture<QJsonObject> jsonObject() const {
//             return json().then([](const QJsonDocument &jsonData) {
//                 return jsonData.object();
//             });
//         }

//         QFuture<QJsonArray> jsonArray() const {
//             return json().then([](const QJsonDocument &jsonData) {
//                 return jsonData.array();
//             });
//         }

//         ~Response() {
//             reply->deleteLater();
//         }
//     };

//     // explicit QClient(QObject *parent = nullptr) : QObject(parent), manager(new QNetworkAccessManager(this)) {}
//     // ~QClient() override = default;

//     static Response get(const QString &url, const QMap<QString, QString> &headers = {}, const QMap<QString, QString> &params = {}) {
//         QNetworkRequest request((QUrl(url)));  // Ensure QUrl is used
//         for (auto it = headers.begin(); it != headers.end(); ++it) {
//             request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
//         }

//         QNetworkReply *reply = getManager()->get(request);
//         return Response(reply);
//     }

//     static Response post(const QString &url, const QMap<QString, QString> &headers = {}, const QMap<QString, QString> &data = {}) {
//         QNetworkRequest request((QUrl(url)));  // Ensure QUrl is used
//         for (auto it = headers.begin(); it != headers.end(); ++it) {
//             request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
//         }

//         QUrlQuery params;
//         for (auto it = data.begin(); it != data.end(); ++it) {
//             params.addQueryItem(it.key(), it.value());
//         }

//         QNetworkReply *reply = getManager()->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
//         return Response(reply);
//     }

// private:
//     static QNetworkAccessManager *getManager() {
//         static QNetworkAccessManager manager;
//         return &manager;
//     }

// };
