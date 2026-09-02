#pragma once

// Idempotent - calling either twice is harmless.
namespace DisplaySleep {
void inhibit();
void allow();
}
