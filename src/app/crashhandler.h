#pragma once
#include <QString>

// Catches only what kills the process from inside; a BSOD or force-close leaves nothing to catch.
namespace CrashHandler {

// Call first thing in main(), before any Qt object exists.
void install();

// Needs Qt running.
void reportPending();

QString crashDir();

}
