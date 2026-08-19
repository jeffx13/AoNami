#pragma once

// Keeps the display awake during playback. Idempotent - calling either twice is harmless.
namespace DisplaySleep {
void inhibit();
void allow();
}
