#include "PercentageView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

constexpr int PercentageView::kStep;

namespace {
constexpr uint32_t kTrackColor = 0x39C7;  // dark grey
const uint16_t kValueColor = ViewColors::toRGB565(ViewColors::Percentage);  // this view's assigned color
const uint16_t kAdjustColor = ViewColors::toRGB565(ViewColors::PercentageSecondary);  // brighter, on-theme
}  // namespace

void PercentageView::begin(const DeviceConfiguration& config) {
    _name = config.name;
    _commandId = config.commandTemplates.empty() ? "" : config.commandTemplates[0].id;
    _reportId = config.reportTemplates.empty() ? "" : config.reportTemplates[0].id;
    _value = 0;
    _pendingValue = 0;
    _adjusting = false;
}

void PercentageView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    if (_reportId.length() == 0) {
        return;
    }

    if (!_adjusting) {
        _adjusting = true;
        _pendingValue = _value;
    } else {
        _adjusting = false;
        _value = _pendingValue;
        Buzzer::confirm();
        publishReport(Report{_reportId, String(_value)});
    }
}

void PercentageView::onEncoderChange(int delta) {
    _pendingValue += (delta > 0 ? kStep : -kStep);
    if (_pendingValue < 0) {
        _pendingValue = 0;
    }
    if (_pendingValue > 100) {
        _pendingValue = 100;
    }
}

void PercentageView::onCommand(const Command& command) {
    if (_adjusting || _commandId.length() == 0 || command.id != _commandId) {
        return;
    }

    int value = command.value.as<int>();
    if (value < 0) {
        value = 0;
    }
    if (value > 100) {
        value = 100;
    }
    _value = value;
}

void PercentageView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int displayValue = _adjusting ? _pendingValue : _value;
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
    canvas.drawString(_adjusting ? "rotate + tap to confirm" : _name, cx, cy + 30);
}

namespace {
struct PercentageViewRegistrar {
    PercentageViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.PercentageView",
                                              []() { return std::make_unique<PercentageView>(); });
    }
} percentageViewRegistrar;
}  // namespace
