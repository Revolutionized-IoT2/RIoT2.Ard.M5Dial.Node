#include "ValueView.h"

#include <memory>

#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kValueColor = ViewColors::toRGB565(ViewColors::Value);           // this view's assigned color
const uint16_t kLabelColor = ViewColors::toRGB565(ViewColors::ValueSecondary);  // pale label accent
}  // namespace

void ValueView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void ValueView::onCommand(const Command& command) {
    _model.onCommand(command);
}

void ValueView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);
    canvas.setTextDatum(middle_center);

    if (_model.size() == 0) {
        canvas.setTextColor(WHITE);
        canvas.setTextSize(2);
        canvas.drawString("No values", canvas.width() / 2, canvas.height() / 2);
        return;
    }

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    if (_model.size() == riot2::views::ValueModel::kMaxSlots) {
        // Exactly 4 values: one on top, two side-by-side in the middle, one
        // on the bottom (compass style) - a plain vertical stack of 4 rows
        // would be too cramped to read on a 240x240 round display.
        struct Position {
            int centerX;
            int rowCenterY;
        };
        const int offsetX = 45;
        const Position positions[riot2::views::ValueModel::kMaxSlots] = {
            {cx, cy - 60},           // top
            {cx - offsetX, cy + 4},  // middle-left
            {cx + offsetX, cy + 4},  // middle-right
            {cx, cy + 68},           // bottom
        };
        for (size_t i = 0; i < _model.size(); ++i) {
            const Position& pos = positions[i];
            canvas.setTextColor(kLabelColor);
            canvas.setTextSize(1);
            canvas.drawString(_model.name(i), pos.centerX, pos.rowCenterY - 14);
            canvas.setTextColor(kValueColor);
            canvas.setTextSize(2);
            canvas.drawString(_model.text(i), pos.centerX, pos.rowCenterY + 10);
        }
        return;
    }

    // Fewer than 4 values: stack them in a centered vertical column.
    int rowHeight = canvas.height() / static_cast<int>(_model.size());
    for (size_t i = 0; i < _model.size(); ++i) {
        int centerY = static_cast<int>(i) * rowHeight + rowHeight / 2;
        canvas.setTextColor(kLabelColor);
        canvas.setTextSize(1);
        canvas.drawString(_model.name(i), cx, centerY - 14);
        canvas.setTextColor(kValueColor);
        canvas.setTextSize(3);
        canvas.drawString(_model.text(i), cx, centerY + 10);
    }
}

namespace {
struct ValueViewRegistrar {
    ValueViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.ValueView", []() { return std::make_unique<ValueView>(); },
            []() { return riot2::views::ValueModel::buildTemplate("RIoT2.Ard.M5Dial.Node.ValueView"); });
    }
} valueViewRegistrar;
}  // namespace
