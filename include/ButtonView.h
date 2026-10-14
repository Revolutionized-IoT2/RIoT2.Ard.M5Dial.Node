#pragma once

#include <Arduino.h>

#include <riot2/views/ButtonModel.h>

#include "IView.h"

// 1-4 momentary buttons (the shared riot2::views::ButtonModel) arranged in
// a centered grid: a single button or a centered pair for 1-2 buttons, a
// 2x2 grid for 3-4. Tapping a button publishes a Report (id -> "true") for
// its reportTemplate and briefly lights it; an inbound Command addressed to
// the matching commandTemplate.id (correlated by shared `address`) sets
// that button's highlighted/on-off visual state. The view's own
// header/subheader text come from the "header"/"subHeader"
// deviceParameters.
class ButtonView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

private:
    riot2::views::ButtonModel _model;

    int slotAt(int x, int y) const;
};
