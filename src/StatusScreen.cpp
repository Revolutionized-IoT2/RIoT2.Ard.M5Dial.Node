#include "StatusScreen.h"

#include <M5Dial.h>

void showStatus(const String& line1, const String& line2) {
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextDatum(middle_center);

    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString(line1, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 10);

    if (line2.length()) {
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.drawString(line2, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 15);
    }
}
