#pragma once
#include <QUrl>
#include <QString>

// Parses a playback target; an empty url falls back to the clipboard (URL or curl command).
namespace UrlParser {

struct ParsedUrl {
    QUrl url;
    QString raw;
    bool valid = false;
};

ParsedUrl parse(QUrl url);

}
