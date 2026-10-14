#include "DiagnosticsScreen.h"

#include <M5Dial.h>
#include <WiFi.h>

#include "Buzzer.h"
#include "StatusScreen.h"
#include "ViewColors.h"

namespace {
constexpr int kShutdownButtonRadius = 20;
constexpr int kShutdownRingInnerRadius = 24;
constexpr int kShutdownRingOuterRadius = 30;
// Generous touch target for this small round button - the round panel makes
// precise hit-circles harder to land than on a square screen (see the
// three-band touch reasoning for the carousel in CLAUDE.md).
constexpr int kShutdownHitRadius = kShutdownRingOuterRadius + 10;
const uint16_t kShutdownAccentColor = ViewColors::toRGB565(ViewColors::Alert);
const uint16_t kShutdownTrackColor = ViewColors::toRGB565(ViewColors::AlertSecondary);

// Center of the shutdown button, shared by rendering and hit-testing. Close
// enough to the display center that its hit-circle stays inside the round
// visible area even offset to the right.
int buttonCx() {
    return M5Dial.Display.width() - 55;
}
int buttonCy() {
    return M5Dial.Display.height() / 2 + 10;
}

bool isNearButton(int x, int y) {
    int dx = x - buttonCx();
    int dy = y - buttonCy();
    return (dx * dx + dy * dy) <= (kShutdownHitRadius * kShutdownHitRadius);
}

// Publishes the graceful-shutdown offline message (CLAUDE.md's "Graceful
// shutdown" lifecycle step) and powers off; only the physical power button
// starts the device again.
void shutdown(riot2::NodeRuntime& runtime) {
    Serial.println("[Power] Shutdown confirmed from diagnostics screen");
    Buzzer::error();
    showStatus("Shutting down...", "Press power button to restart");
    runtime.publishOfflineAndDisconnect();
    delay(300);
    M5Dial.Power.powerOff();
}
}  // namespace

void DiagnosticsScreen::loop(M5Canvas& canvas, riot2::NodeRuntime& runtime, bool touchDown, int touchX, int touchY) {
    if (touchDown && isNearButton(touchX, touchY)) {
        if (!_holding) {
            _holding = true;
            _holdStartMs = millis();
        } else if (millis() - _holdStartMs >= kShutdownHoldMs) {
            shutdown(runtime);  // never returns
        }
    } else {
        _holding = false;
    }
    render(canvas, runtime);
}

void DiagnosticsScreen::render(M5Canvas& target, riot2::NodeRuntime& runtime) {
    target.fillScreen(BLACK);
    target.setTextColor(WHITE);
    target.setTextDatum(middle_center);
    target.setTextSize(2);
    target.drawString("Diagnostics", target.width() / 2, 20);
    target.setTextSize(1);
    target.drawString("hold button to close", target.width() / 2, 38);

    target.setTextDatum(middle_left);
    int x = 20;
    int y = 56;
    const int lineHeight = 16;

    const NodeConfig& config = runtime.config();
    target.drawString("Node: " + config.name, x, y);
    y += lineHeight;
    target.drawString("ID: " + config.id, x, y);
    y += lineHeight;
    target.drawString(String("WiFi: ") + (runtime.wifi().isConnected() ? "connected" : "connecting..."), x, y);
    y += lineHeight;
    if (runtime.wifi().isConnected()) {
        target.drawString("SSID: " + WiFi.SSID(), x, y);
        y += lineHeight;
        target.drawString("RSSI: " + String(WiFi.RSSI()) + " dBm", x, y);
        y += lineHeight;
    }
    target.drawString(String("MQTT: ") + (runtime.mqtt().isConnected() ? "connected" : "connecting..."), x, y);
    y += lineHeight;
    target.drawString("Free heap: " + String(ESP.getFreeHeap() / 1024) + " KB", x, y);
    y += lineHeight;
    target.drawString("Uptime: " + String(millis() / 1000) + " s", x, y);

    int cx = buttonCx();
    int cy = buttonCy();
    float holdFraction = 0.f;
    if (_holding) {
        holdFraction = static_cast<float>(millis() - _holdStartMs) / static_cast<float>(kShutdownHoldMs);
        if (holdFraction > 1.f) {
            holdFraction = 1.f;
        }
    }

    target.fillArc(cx, cy, kShutdownRingOuterRadius, kShutdownRingInnerRadius, 0, 360, kShutdownTrackColor);
    if (holdFraction > 0.f) {
        target.fillArc(cx, cy, kShutdownRingOuterRadius, kShutdownRingInnerRadius, 0, 360.0f * holdFraction,
                       kShutdownAccentColor);
    }
    target.fillCircle(cx, cy, kShutdownButtonRadius, kShutdownAccentColor);

    target.setTextDatum(middle_center);
    target.setTextColor(WHITE);
    target.setTextSize(1);
    target.drawString("hold 2s", cx, cy + kShutdownRingOuterRadius + 12);
    target.drawString("to power off", cx, cy + kShutdownRingOuterRadius + 24);
}
