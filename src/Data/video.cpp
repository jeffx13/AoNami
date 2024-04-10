#include "video.h"
#include <QHashIterator>
#include <sstream>

void Video::addHeader(const QString &key, const QString &value) {
    m_headers[key] = value;
}


QString Video::getHeaders(const QString &keyValueSeparator, const QString &entrySeparator, bool quotedValue) const {
    QString result;
    if (m_headers.isEmpty()) return result;
    QHashIterator<QString, QString> it(m_headers);
    bool first = true;
    while (it.hasNext()) {
        it.next();
        if (!first) {
            result += entrySeparator; // Add separator except before the first element
        } else {
            first = false;
        }
        auto value = quotedValue ? QString("\"%1\"").arg (it.value()) : it.value();
        result += it.key() + keyValueSeparator + value; // Append "key: value"
    }
    return result;
}


