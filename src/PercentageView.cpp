#include "PercentageView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

constexpr int PercentageView::kStep;

namespace {
const uint16_t kTrackColor = ViewColors::toRGB565(ViewColors::PercentageSecondary);  // pale track, per CLAUDE.md
const uint16_t kValueColor = ViewColors::toRGB565(ViewColors::Percentage);  // this view's assigned color
const uint16_t kAdjustColor = ViewColors::toRGB565(ViewColors::lighten(ViewColors::Percentage, 0.5f));  // brighter, on-theme
}  // namespace

void PercentageView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
    _pendingValue = _model.value();
    _adjusting = false;
}

void PercentageView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    if (!_model.canReport()) {
        return;
    }

    if (!_adjusting) {
        _adjusting = true;
        _pendingValue = _model.value();
        return;
    }
    _adjusting = false;
    Report report;
    if (_model.set(_pendingValue, report)) {
        Buzzer::confirm();
        publishReport(report);
    }
}

void PercentageView::onEncoderChange(int delta) {
    _pendingValue = riot2::views::PercentageModel::clamp(_pendingValue + (delta > 0 ? kStep : -kStep));
}

void PercentageView::onCommand(const Command& command) {
    // A command doesn't move the value under the user's finger.
    if (!_adjusting) {
        _model.onCommand(command);
    }
}

void PercentageView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int displayValue = _adjusting ? _pendingValue : _model.value();
    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;
    int outerR = (cx < cy ? cx : cy) - 10;
    int innerR = outerR - 20;

    canvas.fillArc(cx, cy, outerR, innerR, 0, 360, kTrackColor);
    if (displayValue > 0) {
        canvas.fillArc(cx, cy, outerR, innerR, 0, 360.0f * displayValue / 100.0f,
                       _adjusting ? kAdjustColor : kValueColor);
    }

    canvas.setTextColor(WHITE);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(3);
    canvas.drawString(String(displayValue) + "%", cx, cy - 10);

    canvas.setTextSize(1);
    canvas.drawString(_adjusting ? "rotate + tap to confirm" : _model.name(), cx, cy + 30);
}

namespace {
struct PercentageViewRegistrar {
    PercentageViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.PercentageView", []() { return std::make_unique<PercentageView>(); },
            []() { return riot2::views::PercentageModel::buildTemplate("RIoT2.Ard.M5Dial.Node.PercentageView"); });
    }
} percentageViewRegistrar;
}  // namespace
