#pragma once

#include <Arduino.h>

#include <vector>

#include "IView.h"

// Scrollable text list of scenes/presets, one per reportTemplate. Tap the
// view to start browsing, rotate the dial to move the highlight, tap again
// to confirm - publishes a Report (id -> "true") for the selected
// reportTemplate, then returns to the carousel.
class SceneSelectorView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onEncoderChange(int delta) override;
    bool isInteracting() const override { return _browsing; }
    void render(M5Canvas& canvas) override;

private:
    std::vector<ReportTemplate> _items;
    int _selectedIndex = 0;
    int _pendingIndex = 0;
    bool _browsing = false;
};
