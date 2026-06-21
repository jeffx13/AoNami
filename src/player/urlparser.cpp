#include "urlparser.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QRegularExpression>

namespace UrlParser {

ParsedUrl parse(QUrl url) {
    ParsedUrl result;

    if (!url.isEmpty()) {
        result.url = url;
        result.raw = url.toString();
        result.valid = url.isValid();
        return result;
    }

    QString clipboard = QGuiApplication::clipboard()->text().trimmed();

    if (clipboard.startsWith("curl")) {
        static QRegularExpression curlRegex(R"(curl\s+'([^']+)')");
        QRegularExpressionMatch urlMatch = curlRegex.match(clipboard);
        if (urlMatch.hasMatch()) {
            QStringList parts;
            parts << urlMatch.captured(1);
            static QRegularExpression headerRegex(R"(-H\s+'([^']+)')");
            QRegularExpressionMatchIterator it = headerRegex.globalMatch(clipboard);
            while (it.hasNext())
                parts << it.next().captured(1);
            result.raw = parts.join('|');
            result.url = QUrl::fromUserInput(urlMatch.captured(1));
        }
    } else {
        if (clipboard.startsWith('"')) clipboard.remove(0, 1);
        if (clipboard.endsWith('"')) clipboard.chop(1);
        result.url = QUrl::fromUserInput(clipboard);
        result.raw = clipboard;
    }

    result.valid = result.url.isValid();
    return result;
}

}
