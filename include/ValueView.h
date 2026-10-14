#pragma once

#include <Arduino.h>

#include <riot2/views/ValueModel.h>

#include "IView.h"

// Displays up to 4 read-only values with unit labels (the shared
// riot2::views::ValueModel). Values are only ever updated via an inbound
// Command - ValueView never publishes reports. With exactly 4 values
// they're arranged top/left/right/bottom (compass style); with fewer,
// they're stacked in a centered vertical column.
class ValueView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

private:
    riot2::views::ValueModel _model;
};
