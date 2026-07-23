#pragma once

#include <Arduino.h>

#include "IView.h"

// Tap anywhere on the view to enter "adjust" mode, rotate the dial to change
// a 0-100% value (with an arc as visual feedback), tap again to confirm and
// publish the value as a Report. The value can also be pushed in from an
// inbound Command while not actively being adjusted.
class PercentageView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onEncoderChange(int delta) override;
    void onCommand(const Command& command) override;
    bool isInteracting() const override { return _adjusting; }
    void render(M5Canvas& canvas) override;

private:
    static constexpr int kStep = 2;

    String _commandId;
    String _reportId;
    String _name;
    int _value = 0;
    int _pendingValue = 0;
    bool _adjusting = false;
};
