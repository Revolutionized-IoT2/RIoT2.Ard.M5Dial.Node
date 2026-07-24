#pragma once

#include <Arduino.h>

#include <vector>

#include "IView.h"

// 1-4 momentary buttons arranged in a centered grid: a single button or a
// centered pair for 1-2 buttons, a 2x2 grid for 3-4. Tapping a button
// publishes a Report (id -> "true") for its reportTemplate; an inbound
// Command addressed to the matching commandTemplate.id (correlated by
// shared `address`) sets that button's highlighted/on-off visual state.
// The view's own header/subheader text come from the "header"/"subHeader"
// deviceParameters.
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
    String _header;
    String _subHeader;

    int slotAt(int x, int y) const;
};
