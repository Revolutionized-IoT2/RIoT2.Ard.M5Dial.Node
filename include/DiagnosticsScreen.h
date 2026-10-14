#pragma once

#include <Arduino.h>
#include <M5GFX.h>

#include <riot2/NodeRuntime.h>

// The diagnostics screen (node, Wi-Fi, MQTT, heap, uptime) with an on-screen shutdown button:
// touching and holding it for kShutdownHoldMs powers the device off (M5Dial.Power.powerOff(); the
// physical power button starts it again). Lifting the finger or dragging off the button cancels.
class DiagnosticsScreen {
public:
    static constexpr unsigned long kShutdownHoldMs = 2000;

    // Once per loop() while shown: tracks the shutdown hold, then draws.
    void loop(M5Canvas& canvas, riot2::NodeRuntime& runtime, bool touchDown, int touchX, int touchY);

    // Leaving the screen cancels a shutdown hold.
    void hide() { _holding = false; }

private:
    bool _holding = false;
    unsigned long _holdStartMs = 0;

    void render(M5Canvas& canvas, riot2::NodeRuntime& runtime);
};
