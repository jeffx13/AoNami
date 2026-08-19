#pragma once

#include <QString>

namespace Js {

// Unpacks Dean Edwards' p,a,c,k,e,d packed JS from provider embed pages.
QString unpack(const QString &html);

} // namespace Js
