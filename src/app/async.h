#pragma once
#include <QtGlobal>

// An exception escaping waitForFinished() during teardown is std::terminate. Cancellation stays
// at the call site, which cancels every token before waiting on any.
template <typename Waitable>
void waitFor(Waitable &waitable, const char *what) {
    if (!waitable.isRunning()) return;
    try {
        waitable.waitForFinished();
    } catch (...) {
        qWarning("%s threw during shutdown", what);
    }
}
