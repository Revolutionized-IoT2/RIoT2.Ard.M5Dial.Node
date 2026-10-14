#include "DisplayPower.h"

#include <M5Dial.h>

void DisplayPower::begin() {
    _fullBrightness = M5Dial.Display.getBrightness();
    _lastActivityMs = millis();
}

bool DisplayPower::wake() {
    bool wasAsleep = _asleep;
    if (_asleep) {
        M5Dial.Display.wakeup();
        _asleep = false;
    }
    if (_dimmed) {
        M5Dial.Display.setBrightness(_fullBrightness);
        _dimmed = false;
    }
    _lastActivityMs = millis();
    return wasAsleep;
}

void DisplayPower::loop() {
    unsigned long idleMs = millis() - _lastActivityMs;
    if (!_asleep && idleMs >= kSleepTimeoutMs) {
        M5Dial.Display.sleep();
        _asleep = true;
    } else if (!_asleep && !_dimmed && idleMs >= kDimTimeoutMs) {
        M5Dial.Display.setBrightness(kDimBrightness);
        _dimmed = true;
    }
}
