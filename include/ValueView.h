#pragma once

#include <Arduino.h>

#include <vector>

#include "IView.h"

// Displays up to 2 read-only values with unit labels. Values are only ever
// updated via an inbound Command matching one of this view's
// commandTemplates - ValueView never publishes reports.
class ValueView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

private:
    struct Slot {
        String id;
        String name;
        String unit;
        String value = "--";
    };

    std::vector<Slot> _slots;
};
