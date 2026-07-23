#pragma once

#include <Arduino.h>

#include "IView.h"

// A single large on/off switch - a simpler one-device variant of ButtonView,
// good for one light/relay per view. Tap anywhere on the view to flip the
// switch and publish its new state as a Report; an inbound Command
// addressed to the matching commandTemplate.id (correlated via shared
// `address`, as in ButtonView) sets the visual state.
class ToggleView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

private:
    String _name;
    String _reportId;
    String _commandId;
    bool _hasCommand = false;
    bool _active = false;

    void toggle();
};
