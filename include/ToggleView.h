#pragma once

#include <Arduino.h>

#include <vector>

#include "IView.h"

// 1-2 on/off switches, stacked vertically (a single switch is centered on
// screen). Tapping a switch's half of the screen flips it and publishes its
// new state as a Report; an inbound Command addressed to the matching
// commandTemplate.id (correlated via shared `address`, as in ButtonView)
// sets that switch's visual state.
class ToggleView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

private:
    struct Slot {
        ReportTemplate report;
        CommandTemplate command;
        bool hasCommand = false;
        bool active = false;
    };

    std::vector<Slot> _slots;

    int slotAt(int x, int y) const;
    void toggle(int index);
};
