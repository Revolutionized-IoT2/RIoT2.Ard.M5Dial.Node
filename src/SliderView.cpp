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
    _model.begin(config);
    _pendingValue = _model.value();
    _adjusting = false;
}

void SliderView::onTouch(int x, int y) {
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

void SliderView::onEncoderChange(int delta) {
    // On the step grid, so the value shown is the value reported.
    _pendingValue = _model.snap(_pendingValue + (delta > 0 ? _model.step() : -_model.step()));
}

void SliderView::onCommand(const Command& command) {
    // A command doesn't move the slider under the user's finger.
    if (!_adjusting) {
        _model.onCommand(command);
    }
}

void SliderView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int displayValue = _adjusting ? _pendingValue : _model.value();
    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    float fraction = static_cast<float>(displayValue - _model.minimum()) /
                     static_cast<float>(_model.maximum() - _model.minimum());
    uint16_t fillColor = _adjusting ? kAdjustColor : kValueColor;

    canvas.setTextColor(WHITE, BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(2);
    canvas.drawString(_model.name(), cx, cy - 50);

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
    String valueText = String(displayValue) + (_model.unit().length() ? " " + _model.unit() : "");
    canvas.drawString(valueText, cx, cy + 45);

    canvas.setTextSize(1);
    if (_adjusting) {
        canvas.drawString("rotate + tap to confirm", cx, cy + 75);
    }
}

namespace {
struct SliderViewRegistrar {
    SliderViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.SliderView", []() { return std::make_unique<SliderView>(); },
            []() { return riot2::views::SliderModel::buildTemplate("RIoT2.Ard.M5Dial.Node.SliderView"); });
    }
} sliderViewRegistrar;
}  // namespace
