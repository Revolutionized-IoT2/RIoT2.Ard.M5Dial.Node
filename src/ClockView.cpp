#include "ClockView.h"

#include <memory>

#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kAccentColor = ViewColors::toRGB565(ViewColors::Clock);           // this view's assigned color
const uint16_t kSubtleColor = ViewColors::toRGB565(ViewColors::ClockSecondary);  // pale accent for date/status text
}  // namespace

void ClockView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void ClockView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    // Thin themed ring around the clock face, matching this view's carousel icon color.
    int radius = (cx < cy ? cx : cy) - 12;
    canvas.drawCircle(cx, cy, radius, kAccentColor);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE);

    String timeText;
    String dateText;
    bool synced = riot2::views::ClockModel::now(timeText, dateText);
    canvas.setTextSize(synced ? 3 : 2);
    canvas.drawString(timeText, cx, cy - 10);

    canvas.setTextColor(kSubtleColor);
    canvas.setTextSize(1);
    canvas.drawString(dateText, cx, cy + 25);
}

namespace {
struct ClockViewRegistrar {
    ClockViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.ClockView", []() { return std::make_unique<ClockView>(); },
            []() { return riot2::views::ClockModel::buildTemplate("RIoT2.Ard.M5Dial.Node.ClockView"); });
    }
} clockViewRegistrar;
}  // namespace
