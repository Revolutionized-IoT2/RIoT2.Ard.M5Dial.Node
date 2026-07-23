#pragma once

#include <Arduino.h>

#include <vector>

#include "IView.h"

// 1-4 momentary buttons stacked vertically. Tapping a button publishes a
// Report (id -> "true") for its reportTemplate; an inbound Command addressed
// to the matching commandTemplate.id (correlated by shared `address`) sets
// that button's highlighted/on-off visual state.
class ButtonView : public IView {
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
        unsigned long flashUntilMs = 0;
    };

    std::vector<Slot> _slots;

    int slotAt(int x, int y) const;
};
