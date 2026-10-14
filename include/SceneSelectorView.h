#pragma once

#include <Arduino.h>

#include <riot2/views/SceneSelectorModel.h>

#include "IView.h"

// Scrollable text list of scenes/presets (the shared
// riot2::views::SceneSelectorModel), one per reportTemplate. Tap the view to
// start browsing, rotate the dial to move the highlight, tap again to
// confirm - publishes a Report (id -> "true") for the selected
// reportTemplate, then returns to the carousel.
class SceneSelectorView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onEncoderChange(int delta) override;
    bool isInteracting() const override { return _browsing; }
    void render(M5Canvas& canvas) override;

private:
    riot2::views::SceneSelectorModel _model;
    int _selectedIndex = 0;
    int _pendingIndex = 0;
    bool _browsing = false;
};
