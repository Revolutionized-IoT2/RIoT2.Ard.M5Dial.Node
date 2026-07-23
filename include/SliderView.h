#pragma once

#include <Arduino.h>

#include "IView.h"

// Like PercentageView but for an arbitrary numeric range + unit (not just
// 0-100%), e.g. brightness, volume, fan speed. Range is configured via
// deviceParameters: "min", "max", "step", "unit" (all optional, default to
// 0, 100, 1, ""). Renders as a horizontal pill-shaped track with decorative
// tick dots and a thumb divider (Android-style volume slider), rather than
// PercentageView's circular arc gauge. Tap anywhere on the view to enter
// "adjust" mode, rotate the dial to change the value, tap again to confirm
// and publish it as a Report. The value can also be pushed in from an
// inbound Command while idle.
class SliderView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onEncoderChange(int delta) override;
    void onCommand(const Command& command) override;
    bool isInteracting() const override { return _adjusting; }
    void render(M5Canvas& canvas) override;

private:
    String _commandId;
    String _reportId;
    String _name;
    String _unit;
    int _min = 0;
    int _max = 100;
    int _step = 1;
    int _value = 0;
    int _pendingValue = 0;
    bool _adjusting = false;

    int clamp(int value) const;
};
