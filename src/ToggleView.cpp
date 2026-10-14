#include "ToggleView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kLabelColor = ViewColors::toRGB565(ViewColors::Toggle);            // this view's assigned color
const uint16_t kOnColor = ViewColors::toRGB565(ViewColors::Toggle);               // primary "on" track, per CLAUDE.md
const uint16_t kOffColor = ViewColors::toRGB565(ViewColors::ToggleSecondary);     // pale secondary "off" track, per CLAUDE.md
constexpr int kSwitchWidth = 100;
constexpr int kSwitchHeight = 44;

void drawSwitch(M5Canvas& canvas, int cx, int cy, const String& name, bool active) {
    int trackX = cx - kSwitchWidth / 2;
    int trackY = cy - kSwitchHeight / 2;
    int radius = kSwitchHeight / 2;

    canvas.setTextColor(kLabelColor, BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(2);
    canvas.drawString(name, cx, cy - kSwitchHeight / 2 - 24);

    canvas.fillRoundRect(trackX, trackY, kSwitchWidth, kSwitchHeight, radius, active ? kOnColor : kOffColor);

    int knobRadius = radius - 4;
    int knobX = active ? trackX + kSwitchWidth - radius : trackX + radius;
    canvas.fillCircle(knobX, cy, knobRadius, WHITE);
}
}  // namespace

void ToggleView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

int ToggleView::slotAt(int x, int y) const {
    (void)x;
    int count = static_cast<int>(_model.size());
    if (count == 0) {
        return -1;
    }
    if (count == 1) {
        return 0;
    }
    int index = y / (240 / count);
    return index < 0 ? 0 : (index >= count ? count - 1 : index);
}

void ToggleView::onTouch(int x, int y) {
    int index = slotAt(x, y);
    Report report;
    if (index >= 0 && _model.toggle(static_cast<size_t>(index), report)) {
        Buzzer::tap();
        publishReport(report);
    }
}

void ToggleView::onCommand(const Command& command) {
    _model.onCommand(command);
}

void ToggleView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    if (_model.size() == 0) {
        canvas.setTextColor(WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(2);
        canvas.drawString("No switches", canvas.width() / 2, canvas.height() / 2);
        return;
    }

    int cx = canvas.width() / 2;
    if (_model.size() == 1) {
        drawSwitch(canvas, cx, canvas.height() / 2, _model.name(0), _model.isActive(0));
        return;
    }
    int rowHeight = canvas.height() / static_cast<int>(_model.size());
    for (size_t i = 0; i < _model.size(); ++i) {
        int cy = static_cast<int>(i) * rowHeight + rowHeight / 2;
        drawSwitch(canvas, cx, cy, _model.name(i), _model.isActive(i));
    }
}

namespace {
struct ToggleViewRegistrar {
    ToggleViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.ToggleView", []() { return std::make_unique<ToggleView>(); },
            []() { return riot2::views::ToggleModel::buildTemplate("RIoT2.Ard.M5Dial.Node.ToggleView", 1); });
    }
} toggleViewRegistrar;
}  // namespace
