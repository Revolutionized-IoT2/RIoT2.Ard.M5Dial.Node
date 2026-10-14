#pragma once

#include <Arduino.h>

#include <riot2/views/ColorSchemeModel.h>

#include "IView.h"

// Two-stage color picker:
//  1. Tap the center swatch to start picking.
//  2. Rotate the dial to step through 12 fixed main colors - the center
//     swatch previews the chosen one live (as a pure color), and the outer
//     rim continuously redraws to show that color's shades (a black ->
//     color -> white sweep).
//  3. Tap the center again to confirm the pure main color, OR tap a point
//     on the outer rim to confirm that specific shade (the center swatch
//     then shows the chosen shade too). Either tap publishes the result as
//     a "#RRGGBB" hex string Report and returns to the idle preview.
// The color and the picking state live in the shared
// riot2::views::ColorSchemeModel (shade picker).
class ColorSchemeView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onEncoderChange(int delta) override;
    void onCommand(const Command& command) override;
    bool isInteracting() const override { return _model.isPicking(); }
    void render(M5Canvas& canvas) override;

private:
    riot2::views::ColorSchemeModel _model{riot2::views::ColorSchemeModel::Picker::Shade};
};
