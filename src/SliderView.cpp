#include "SliderView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewFactory.h"

namespace {
constexpr uint32_t kTrackColor = 0x39C7;   // dark grey
constexpr uint32_t kValueColor = 0x07FF;   // cyan
constexpr uint32_t kAdjustColor = 0xFFE0;  // yellow
}  // namespace

void SliderView::begin(const DeviceConfiguration& config) {
    _name = config.name;
    _commandId = config.commandTemplates.empty() ? "" : config.commandTemplates[0].id;
    _reportId = config.reportTemplates.empty() ? "" : config.reportTemplates[0].id;

    _min = findParameter(config.deviceParameters, "min", "0").toInt();
    _max = findParameter(config.deviceParameters, "max", "100").toInt();
    _step = findParameter(config.deviceParameters, "step", "1").toInt();
    if (_step <= 0) {
        _step = 1;
    }
    if (_max <= _min) {
        _max = _min + 1;
    }
    _unit = findParameter(config.deviceParameters, "unit", "");

    _value = clamp(_min);
    _pendingValue = _value;
    _adjusting = false;
}

int SliderView::clamp(int value) const {
    if (value < _min) {
        return _min;
    }
    if (value > _max) {
        return _max;
    }
    return value;
}

void SliderView::onTouch(int x, int y) {
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

void SliderView::onEncoderChange(int delta) {
    _pendingValue = clamp(_pendingValue + (delta > 0 ? _step : -_step));
}

void SliderView::onCommand(const Command& command) {
    if (_adjusting || _commandId.length() == 0 || command.id != _commandId) {
        return;
    }
    _value = clamp(command.value.as<int>());
}

void SliderView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int displayValue = _adjusting ? _pendingValue : _value;
    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;
    int outerR = (cx < cy ? cx : cy) - 10;
    int innerR = outerR - 20;

    float fraction = static_cast<float>(displayValue - _min) / static_cast<float>(_max - _min);

    canvas.fillArc(cx, cy, outerR, innerR, 0, 360, kTrackColor);
    if (fraction > 0) {
        canvas.fillArc(cx, cy, outerR, innerR, 0, 360.0f * fraction, _adjusting ? kAdjustColor : kValueColor);
    }

    canvas.setTextColor(WHITE);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(3);
    String valueText = String(displayValue) + (_unit.length() ? " " + _unit : "");
    canvas.drawString(valueText, cx, cy - 10);

    canvas.setTextSize(1);
    canvas.drawString(_adjusting ? "rotate + tap to confirm" : _name, cx, cy + 30);
}

namespace {
struct SliderViewRegistrar {
    SliderViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.SliderView",
                                              []() { return std::make_unique<SliderView>(); });
    }
} sliderViewRegistrar;
}  // namespace
