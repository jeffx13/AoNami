#pragma once

#include <QString>

namespace Functions {

// Unpacks Dean Edwards' p,a,c,k,e,d packed JS from provider embed pages.
QString jsUnpack(const QString &html);

} // namespace Functions
