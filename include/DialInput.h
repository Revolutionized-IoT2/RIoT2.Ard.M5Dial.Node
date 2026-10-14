#pragma once

#include <Arduino.h>
#include <ESP32Encoder.h>

// The M5Dial's inputs for one loop() iteration: bezel clicks, the physical button's gestures and
// the touch panel.
class DialInput {
public:
    // Long press on the physical button (BtnA): factory reset.
    static constexpr unsigned long kFactoryResetHoldMs = 5000;
    // Medium press: toggles the diagnostics screen.
    static constexpr unsigned long kDiagnosticsHoldMs = 1500;
    // The encoder reports several raw quadrature counts per physical detent (one click moved the
    // carousel by 4 items before this was accounted for), so counts become whole clicks here.
    static constexpr int kCountsPerClick = 4;

    struct Frame {
        int encoderClicks = 0;
        // Released before kDiagnosticsHoldMs, and not used by a hold.
        bool buttonTapped = false;
        bool toggleDiagnostics = false;
        bool factoryReset = false;
        bool touched = false;    // touch started this frame
        bool touchDown = false;  // finger on the panel
        int touchX = 0;
        int touchY = 0;

        bool hasInput() const { return encoderClicks != 0 || buttonTapped || touched; }
    };

    // After M5Dial.begin(cfg, false, false): attaches the hardware encoder.
    void begin();

    // Once per loop(), after M5Dial.update().
    Frame read();

private:
    // M5Dial.Encoder decodes quadrature in software with pin-change ISRs, which miss edges on fast
    // spins while Wi-Fi/BT, flash or rendering hold off interrupts ("skips a beat"). The ESP32's
    // PCNT peripheral counts in hardware, so it can't drop counts; M5Dial.begin() is told
    // enableEncoder=false so the software decoder never attaches to the same pins.
    ESP32Encoder _encoder;
    long _lastPosition = 0;
    int _remainder = 0;
    unsigned long _pressStartMs = 0;
    bool _diagnosticsHoldFired = false;
    bool _factoryResetFired = false;
};
