#pragma once

#include <Arduino.h>

#include <riot2/views/SliderModel.h>

#include "IView.h"

// Like PercentageView but for an arbitrary numeric range + unit (not just
// 0-100%), e.g. brightness, volume, fan speed: the shared
// riot2::views::SliderModel (deviceParameters "min", "max", "step", "unit").
// Renders as a horizontal pill-shaped track with a thumb (Android-style
// volume slider). Tap anywhere on the view to enter "adjust" mode, rotate
// the dial to change the value, tap again to confirm and publish it as a
// Report. The value can also be pushed in from an inbound Command while
// idle.
class SliderView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onEncoderChange(int delta) override;
    void onCommand(const Command& command) override;
    bool isInteracting() const override { return _adjusting; }
    void render(M5Canvas& canvas) override;

private:
    riot2::views::SliderModel _model;
    int _pendingValue = 0;
    bool _adjusting = false;
};
