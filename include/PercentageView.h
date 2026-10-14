#pragma once

#include <Arduino.h>

#include <riot2/views/PercentageModel.h>

#include "IView.h"

// Tap anywhere on the view to enter "adjust" mode, rotate the dial to change
// a 0-100% value (the shared riot2::views::PercentageModel, with an arc as
// visual feedback), tap again to confirm and publish the value as a Report.
// The value can also be pushed in from an inbound Command while not actively
// being adjusted.
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

    riot2::views::PercentageModel _model;
    int _pendingValue = 0;
    bool _adjusting = false;
};
