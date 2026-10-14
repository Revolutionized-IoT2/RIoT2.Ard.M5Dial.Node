#include "DialInput.h"

#include <M5Dial.h>

void DialInput::begin() {
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    _encoder.attachFullQuad(DIAL_ENCODER_PIN_A, DIAL_ENCODER_PIN_B);
    _encoder.setCount(0);
}

DialInput::Frame DialInput::read() {
    Frame frame;

    if (!_factoryResetFired && M5Dial.BtnA.pressedFor(kFactoryResetHoldMs)) {
        _factoryResetFired = true;
        frame.factoryReset = true;
    }
    if (!_diagnosticsHoldFired && M5Dial.BtnA.pressedFor(kDiagnosticsHoldMs)) {
        _diagnosticsHoldFired = true;
        frame.toggleDiagnostics = true;
    }
    if (M5Dial.BtnA.wasPressed()) {
        _pressStartMs = millis();
    }
    // The physical button only ever returns to the carousel (ViewManager::onButtonPress()); views
    // are driven by touch and the bezel.
    if (M5Dial.BtnA.wasReleased()) {
        frame.buttonTapped = !_diagnosticsHoldFired && (millis() - _pressStartMs) < kDiagnosticsHoldMs;
        _diagnosticsHoldFired = false;
    }

    long position = _encoder.getCount();
    int rawDelta = static_cast<int>(position - _lastPosition);
    _lastPosition = position;
    if (rawDelta != 0) {
        // Drop the previous direction's partial click when the wheel reverses; otherwise a reversal
        // first pays down the old remainder and seems to take two clicks.
        if ((rawDelta > 0 && _remainder < 0) || (rawDelta < 0 && _remainder > 0)) {
            _remainder = 0;
        }
        _remainder += rawDelta;
        frame.encoderClicks = _remainder / kCountsPerClick;
        _remainder -= frame.encoderClicks * kCountsPerClick;
    }

    if (M5Dial.Touch.getCount()) {
        auto detail = M5Dial.Touch.getDetail(0);
        frame.touchX = detail.x;
        frame.touchY = detail.y;
        frame.touchDown = detail.isPressed();
        frame.touched = detail.wasPressed();
    }
    return frame;
}
