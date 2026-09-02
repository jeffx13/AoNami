#pragma once
#include <QtGlobal>

// An exception escaping waitForFinished() at teardown is std::terminate.
template <typename Waitable>
void waitFor(Waitable &waitable, const char *what) {
    if (!waitable.isRunning()) return;
    try {
        waitable.waitForFinished();
    } catch (...) {
        qWarning("%s threw during shutdown", what);
    }
}
