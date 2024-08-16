// #include "vidsrcextractor.h"
// #include <network/network.h>
// #include <QUrlQuery>
// #define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
// #include <QUrl>
// #include <cryptopp/arc4.h>


// #include <cryptopp/filters.h>
// #include <cryptopp/base64.h>

// Vidsrcextractor::Vidsrcextractor() {
//     keys = Client::get(keysJsonUrl).toJsonArray();
// }

// QVector<Video> Vidsrcextractor::videosFromUrl(QString embedLink, QString hosterName, QString type, QVector<SubTrack> subtitleList) {
//     auto host = QUrl(embedLink).host();
//     auto apiUrl = getApiUrl(embedLink, keys);

//     QMap<QString, QString> apiHeaders;
//     apiHeaders.insert("Accept", "application/json, text/javascript, */*; q=0.01");
//     apiHeaders.insert("Host", host);
//     apiHeaders.insert("Referer", QUrl::fromPercentEncoding(embedLink.toUtf8()));
//     apiHeaders.insert("X-Requested-With", "XMLHttpRequest");

//     auto sources = Client::get(apiUrl, apiHeaders).toJsonObject()["result"].toObject()["sources"].toArray();
//     QVector<Video> videos;
//     for (const auto &source : sources) {
//         auto file = source.toObject()["file"].toString();
//         videos.emplaceBack(QUrl(file));
//     }
//     return videos;
// }

// QString Vidsrcextractor::getApiUrl(const QString &embedLink, const QJsonArray &keyList) const {
//     auto host = QUrl(embedLink).host();

//     QUrl url(embedLink);
//     QUrlQuery query(url);

//     QStringList paramList;
//     const auto queryItems = query.queryItems();
//     for (const auto &item : queryItems) {
//         paramList.append(item.first + "=" + item.second);
//     }

//     auto vidId = embedLink.mid(embedLink.lastIndexOf('/') + 1).section('?', 0, 0);
//     auto encodedID = encodeID(vidId, keyList);
//     auto apiSlug = callFromFuToken(host, encodedID, embedLink);
//     auto apiUrl = QString("https://%1/%2").arg(host, apiSlug);
//     if (!paramList.isEmpty()) {
//         apiUrl += "?" + paramList.join("&");
//     }
//     return apiUrl;

// }

// QString Vidsrcextractor::encodeID(const QString &videoID, const QJsonArray &keyList) const
// {
//     using namespace CryptoPP;

//     // Ensure keyList has at least two keys
//     if (keyList.size() < 2) {
//         throw std::invalid_argument("keyList must contain at least two keys");
//     }

//     // RC4 Keys
//     QByteArray rc4Key1 = keyList[0].toString().toUtf8();
//     QByteArray rc4Key2 = keyList[1].toString().toUtf8();
//     SecByteBlock key1(reinterpret_cast<const byte*>(rc4Key1.data()), rc4Key1.size());
//     SecByteBlock key2(reinterpret_cast<const byte*>(rc4Key2.data()), rc4Key2.size());

//     // RC4 Decryption
//     Weak::ARC4::Decryption rc4Decryption1(key1, key1.size());
//     Weak::ARC4::Decryption rc4Decryption2(key2, key2.size());

//     // Encrypt the videoID with the first key
//     std::string decryptedText1;
//     StringSource ss1(videoID.toStdString(), true,
//                      new StreamTransformationFilter(rc4Decryption1,
//                                                     new StringSink(decryptedText1)
//                                                     )
//                      );

//     // Encrypt the result with the second key
//     std::string decryptedText2;
//     StringSource ss2(decryptedText1, true,
//                      new StreamTransformationFilter(rc4Decryption2,
//                                                     new StringSink(decryptedText2)
//                                                     )
//                      );

//     // Convert the final decrypted text to QByteArray
//     QByteArray encoded = QByteArray::fromStdString(decryptedText2);

//     // Base64 encode
//     QString base64Encoded = base64Encode(encoded);

//     return base64Encoded;
// }

// QString Vidsrcextractor::callFromFuToken(const QString &host, const QString &data, const QString &embedLink) const {
//     QMap<QString, QString> refererHeaders ;
//     refererHeaders.insert("Referer", embedLink);
//     auto response = Client::get(QString("https://%1/futoken").arg(host), refererHeaders).body;
//     auto fuTokenScript = response.mid(response.indexOf("window") + QString("window").length());
//     fuTokenScript = fuTokenScript.mid(fuTokenScript.indexOf("function") + QString("function").length());
//     fuTokenScript = fuTokenScript.remove("jQuery.ajax(");
//     fuTokenScript = fuTokenScript.left(fuTokenScript.indexOf("+location.search"));
//     auto js = QString("(function%1}(\"%2\"))").arg(fuTokenScript, data);
//     QJSEngine engine;
//     QJSValue result = engine.evaluate(js);
//     if (result.isError()) {
//         qWarning() << "Uncaught exception at line" << result.property("lineNumber").toInt() << ":" << result.toString();
//         return QString();
//     }

//     return result.toString();
// }

// QString Vidsrcextractor::base64Encode(const QByteArray &input) const
// {
//     using namespace CryptoPP;

//     std::string encoded;
//     StringSource ss(reinterpret_cast<const byte*>(input.data()), input.size(), true,
//                     new Base64Encoder(
//                         new StringSink(encoded),
//                         false // No padding
//                         )
//                     );

//     return QString::fromStdString(encoded).replace("/", "_").trimmed();
// }
