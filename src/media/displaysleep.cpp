#include "media/displaysleep.h"
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>

void DisplaySleep::inhibit() { SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED); }
void DisplaySleep::allow()   { SetThreadExecutionState(ES_CONTINUOUS); }

#elif defined(Q_OS_MACOS)
#include <IOKit/pwr_mgt/IOPMLib.h>

namespace {
IOPMAssertionID g_assertion = 0;
}

void DisplaySleep::inhibit() {
    if (g_assertion) return;
    if (IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep,
                                    kIOPMAssertionLevelOn, CFSTR("AoNami playback"),
                                    &g_assertion) != kIOReturnSuccess)
        g_assertion = 0;
}

void DisplaySleep::allow() {
    if (!g_assertion) return;
    IOPMAssertionRelease(g_assertion);
    g_assertion = 0;
}

#else

void DisplaySleep::inhibit() {}
void DisplaySleep::allow()   {}

#endif
