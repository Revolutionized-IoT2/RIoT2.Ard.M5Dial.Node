#pragma once

#include <Arduino.h>

// Draws one or two centered lines straight to the display (start, provisioning, OTA and
// shutdown messages), outside the carousel's canvas.
void showStatus(const String& line1, const String& line2 = "");
