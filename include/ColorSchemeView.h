#pragma once

#include <Arduino.h>

#include "IView.h"

// Cycle through a fixed palette of color swatches with the dial (while
// "picking"), tap the view to confirm and publish the selected color's name
// as a Report. Tap once to start picking, tap again to confirm and return
// to the carousel.
class ColorSchemeView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onEncoderChange(int delta) override;
    void onCommand(const Command& command) override;
    bool isInteracting() const override { return _picking; }
    void render(M5Canvas& canvas) override;

private:
    String _reportId;
    String _commandId;
    int _selectedIndex = 0;
    int _pendingIndex = 0;
    bool _picking = false;
};
