#pragma once

#include <Arduino.h>

// Dims the panel after kDimTimeoutMs without input and puts it to sleep after kSleepTimeoutMs.
// Any input restores it; the first input that wakes a sleeping panel is swallowed (not forwarded
// to the active view), as usual for smart-home dials.
class DisplayPower {
public:
    static constexpr unsigned long kDimTimeoutMs = 15000;
    static constexpr unsigned long kSleepTimeoutMs = 60000;
    static constexpr uint8_t kDimBrightness = 15;

    // After M5Dial.begin(): remembers the full brightness.
    void begin();

    // Restores full brightness and wakes the panel. Returns true if it was asleep, so the input
    // that woke it should be ignored.
    bool wake();

    // Once per loop(): dims or sleeps the panel after the idle timeouts.
    void loop();

private:
    uint8_t _fullBrightness = 128;
    bool _dimmed = false;
    bool _asleep = false;
    unsigned long _lastActivityMs = 0;
};
