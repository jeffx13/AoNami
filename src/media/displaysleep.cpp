#include "media/displaysleep.h"
#include <windows.h>

void DisplaySleep::inhibit() { SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED); }
void DisplaySleep::allow()   { SetThreadExecutionState(ES_CONTINUOUS); }
