#include "showprovider.h"
#include "app/logger.h"
#include <QRegularExpression>
#include <algorithm>
#include <numeric>

float ShowProvider::resolveTitleNumber(QString &title) const {
    if (title.startsWith(QStringLiteral("第"))) {
        static const QRegularExpression re(QStringLiteral("\\d+"));
        title = re.match(title).captured(0);
    }
    bool ok;
    float number = title.toFloat(&ok);
    if (ok)
        title = QString::number(number);
    return ok ? number : -1.0f;
}
