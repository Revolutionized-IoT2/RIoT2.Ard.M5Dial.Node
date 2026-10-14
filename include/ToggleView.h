#pragma once

#include <Arduino.h>

#include <riot2/views/ToggleModel.h>

#include "IView.h"

// 1-2 on/off switches (the shared riot2::views::ToggleModel), stacked
// vertically (a single switch is centered on screen). Tapping a switch's
// half of the screen flips it and publishes its new state as a Report; an
// inbound Command sets that switch's visual state.
class ToggleView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

private:
    riot2::views::ToggleModel _model;

    int slotAt(int x, int y) const;
};
