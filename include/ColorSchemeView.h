#pragma once

#include <Arduino.h>

#include "IView.h"

// Two-stage color picker:
//  1. Tap the center swatch to start picking.
//  2. Rotate the dial to step through 16 fixed main colors - the center
//     swatch previews the chosen one live (as a pure color), and the outer
//     rim continuously redraws to show that color's shades (a black ->
//     color -> white sweep).
//  3. Tap the center again to confirm the pure main color, OR tap a point
//     on the outer rim to confirm that specific shade (the center swatch
//     then shows the chosen shade too). Either tap publishes the result as
//     a "#RRGGBB" hex string Report and returns to the idle preview.
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
    float _hue = 0.0f;        // selected main hue, degrees 0-359
    float _shadeT = 0.5f;     // selected shade position: 0=black, 0.5=pure hue, 1=white
    float _pendingHue = 0.0f;  // live hue while picking (one of the 16 main colors)
    int _pendingColorIndex = 0;  // index (0-15) of the pending main color
    bool _picking = false;
};
