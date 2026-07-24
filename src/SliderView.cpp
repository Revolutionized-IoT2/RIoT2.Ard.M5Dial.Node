#include "SliderView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kTrackColor = ViewColors::toRGB565(ViewColors::SliderSecondary);  // pale track, per CLAUDE.md
const uint16_t kValueColor = ViewColors::toRGB565(ViewColors::Slider);  // this view's assigned color
const uint16_t kAdjustColor = ViewColors::toRGB565(ViewColors::lighten(ViewColors::Slider, 0.5f));  // brighter, on-theme
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

    float fraction = static_cast<float>(displayValue - _min) / static_cast<float>(_max - _min);
    uint16_t fillColor = _adjusting ? kAdjustColor : kValueColor;

    canvas.setTextColor(WHITE, BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(2);
    canvas.drawString(_name, cx, cy - 50);

    // Horizontal pill-shaped track (like a classic volume slider): dark
    // unfilled background, a filled portion in the view's accent color, and
    // a round knob thumb - bigger than the track itself - marking the
    // current value.
    int barWidth = canvas.width() - 80;
    int barHeight = 20;
    int barX = cx - barWidth / 2;
    int barY = cy - barHeight / 2;
    int radius = barHeight / 2;

    canvas.fillRoundRect(barX, barY, barWidth, barHeight, radius, kTrackColor);

    int filledWidth = static_cast<int>(barWidth * fraction + 0.5f);
    filledWidth = filledWidth < 0 ? 0 : (filledWidth > barWidth ? barWidth : filledWidth);
    if (filledWidth > 0) {
        canvas.fillRoundRect(barX, barY, filledWidth, barHeight, radius, fillColor);
    }

    // Round knob thumb at the value boundary, clamped so its center stays
    // within the track's endpoints (it visually overhangs the pale/filled
    // caps since its radius is larger than the track's half-height).
    int thumbRadius = radius + 8;
    int thumbX = barX + filledWidth;
    thumbX = thumbX < barX + thumbRadius ? barX + thumbRadius
                                          : (thumbX > barX + barWidth - thumbRadius ? barX + barWidth - thumbRadius : thumbX);
    canvas.fillCircle(thumbX, cy, thumbRadius, fillColor);
    canvas.drawCircle(thumbX, cy, thumbRadius, kTrackColor);

    canvas.setTextSize(2);
    String valueText = String(displayValue) + (_unit.length() ? " " + _unit : "");
    canvas.drawString(valueText, cx, cy + 45);

    canvas.setTextSize(1);
    if (_adjusting) {
        canvas.drawString("rotate + tap to confirm", cx, cy + 75);
    }
}

namespace {
struct SliderViewRegistrar {
    SliderViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.SliderView",
                                              []() { return std::make_unique<SliderView>(); });
    }
} sliderViewRegistrar;
}  // namespace
