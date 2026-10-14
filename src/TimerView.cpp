#include "TimerView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

using riot2::views::TimerModel;

namespace {
const uint16_t kAccentColor = ViewColors::toRGB565(ViewColors::Timer);           // this view's assigned color
const uint16_t kTrackColor = ViewColors::toRGB565(ViewColors::TimerSecondary);  // pale track
}  // namespace

void TimerView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void TimerView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    if (_model.start()) {
        Buzzer::confirm();
    } else if (_model.cancel() || _model.dismiss()) {
        Buzzer::tap();
    }
}

void TimerView::onEncoderChange(int delta) {
    _model.adjust(delta);
}

void TimerView::onCommand(const Command& command) {
    _model.onCommand(command);
}

void TimerView::loop() {
    if (_model.finishIfElapsed()) {
        if (!_model.beepOnComplete()) {
            Buzzer::confirm();
        }
        Report report;
        if (_model.completionReport(report)) {
            publishReport(report);
        }
    }
    if (_model.ringDue()) {
        Buzzer::ring();
    }
}

void TimerView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    if (_model.phase() == TimerModel::Phase::Running) {
        long remaining = _model.remainingSeconds();
        long total = _model.totalSeconds();

        int outerR = (cx < cy ? cx : cy) - 10;
        int innerR = outerR - 20;
        float fraction = total > 0 ? static_cast<float>(remaining) / static_cast<float>(total) : 0.f;

        canvas.fillArc(cx, cy, outerR, innerR, 0, 360, kTrackColor);
        if (fraction > 0.f) {
            canvas.fillArc(cx, cy, outerR, innerR, 0, 360.0f * fraction, kAccentColor);
        }

        canvas.setTextColor(WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(3);
        canvas.drawString(_model.remainingText(), cx, cy - 10);

        canvas.setTextSize(1);
        canvas.drawString("tap to cancel", cx, cy + 30);
        return;
    }

    if (_model.phase() == TimerModel::Phase::Done) {
        canvas.fillScreen(kAccentColor);
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(WHITE, kAccentColor);
        canvas.setTextSize(3);
        canvas.drawString("Time's up!", cx, cy);
        canvas.setTextSize(1);
        canvas.drawString("tap to dismiss", cx, cy + 30);
        return;
    }

    // Phase::Setting - rotate the dial to choose a duration, tap to start.
    canvas.setTextColor(WHITE);
    canvas.setTextDatum(middle_center);
    canvas.setTextSize(1);
    canvas.drawString(_model.name(), cx, cy - 60);

    canvas.setTextColor(kAccentColor);
    canvas.setTextSize(4);
    canvas.drawString(String(_model.minutes()) + " min", cx, cy);

    canvas.setTextColor(WHITE);
    canvas.setTextSize(1);
    canvas.drawString("rotate to set, tap to start", cx, cy + 40);
}

namespace {
struct TimerViewRegistrar {
    TimerViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.TimerView", []() { return std::make_unique<TimerView>(); },
            []() { return TimerModel::buildTemplate("RIoT2.Ard.M5Dial.Node.TimerView", false); });
    }
} timerViewRegistrar;
}  // namespace
