#include "SliderView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kTrackColor = ViewColors::toRGB565(ViewColors::lighten(ViewColors::Slider, 0.8f));  // pale track
const uint16_t kValueColor = ViewColors::toRGB565(ViewColors::Slider);  // this view's assigned color
const uint16_t kAdjustColor = ViewColors::toRGB565(ViewColors::lighten(ViewColors::Slider, 0.5f));  // brighter, on-theme
const uint16_t kTickColor = ViewColors::toRGB565(ViewColors::lighten(ViewColors::Slider, 0.4f));  // tick dots on track
constexpr int kTickCount = 6;  // decorative tick marks spread across the track, like an Android volume slider
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

    // Horizontal pill-shaped track (like an Android volume slider): pale
    // background, decorative evenly-spaced tick dots, and a filled portion
    // in the view's accent color with a thumb divider at the boundary.
    int barWidth = canvas.width() - 80;
    int barHeight = 28;
    int barX = cx - barWidth / 2;
    int barY = cy - barHeight / 2;
    int radius = barHeight / 2;

    canvas.fillRoundRect(barX, barY, barWidth, barHeight, radius, kTrackColor);

    for (int i = 1; i <= kTickCount; i++) {
        int tickX = barX + radius + (barWidth - 2 * radius) * i / (kTickCount + 1);
        canvas.fillCircle(tickX, cy, 2, kTickColor);
    }

    int filledWidth = static_cast<int>(barWidth * fraction + 0.5f);
    filledWidth = filledWidth < 0 ? 0 : (filledWidth > barWidth ? barWidth : filledWidth);
    if (filledWidth > 0) {
        canvas.fillRoundRect(barX, barY, filledWidth, barHeight, radius, fillColor);
    }

    // Thumb: a short vertical bar marking the boundary between filled and
    // unfilled track.
    int thumbX = barX + filledWidth;
    thumbX = thumbX < barX + 2 ? barX + 2 : (thumbX > barX + barWidth - 2 ? barX + barWidth - 2 : thumbX);
    canvas.fillRoundRect(thumbX - 2, barY - 6, 4, barHeight + 12, 2, WHITE);

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
