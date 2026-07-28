#include "TimerView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kAccentColor = ViewColors::toRGB565(ViewColors::Timer);           // this view's assigned color
const uint16_t kTrackColor = ViewColors::toRGB565(ViewColors::TimerSecondary);  // pale track
constexpr int kRingCount = 6;                  // number of beeps in the "egg timer ring" pattern
constexpr unsigned long kRingIntervalMs = 260;  // spacing between those beeps
}  // namespace

void TimerView::begin(const DeviceConfiguration& config) {
    _name = config.name;
    _commandId = config.commandTemplates.empty() ? "" : config.commandTemplates[0].id;
    _reportId = config.reportTemplates.empty() ? "" : config.reportTemplates[0].id;

    _stepMinutes = findParameter(config.deviceParameters, "stepMinutes", "1").toInt();
    if (_stepMinutes <= 0) {
        _stepMinutes = 1;
    }
    _maxMinutes = findParameter(config.deviceParameters, "maxMinutes", "60").toInt();
    if (_maxMinutes < _stepMinutes) {
        _maxMinutes = _stepMinutes;
    }
    _minutes = findParameter(config.deviceParameters, "defaultMinutes", "5").toInt();
    if (_minutes < _stepMinutes) {
        _minutes = _stepMinutes;
    }
    if (_minutes > _maxMinutes) {
        _minutes = _maxMinutes;
    }
    _beepOnComplete = findParameter(config.deviceParameters, "beepOnComplete", "false").equalsIgnoreCase("true");

    _phase = Phase::Setting;
    _totalSeconds = 0;
}

void TimerView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    switch (_phase) {
        case Phase::Setting:
            start();
            break;
        case Phase::Running:
            cancel();
            break;
        case Phase::Done:
            _phase = Phase::Setting;
            _ringsRemaining = 0;  // stop any in-progress "egg timer ring" beeps
            Buzzer::tap();
            break;
    }
}

void TimerView::onEncoderChange(int delta) {
    if (_phase != Phase::Setting) {
        return;
    }
    _minutes += (delta > 0 ? _stepMinutes : -_stepMinutes);
    if (_minutes < _stepMinutes) {
        _minutes = _stepMinutes;
    }
    if (_minutes > _maxMinutes) {
        _minutes = _maxMinutes;
    }
}

void TimerView::onCommand(const Command& command) {
    if (_phase != Phase::Setting || _commandId.length() == 0 || command.id != _commandId) {
        return;
    }
    int minutes = command.value.as<int>();
    if (minutes < _stepMinutes) {
        minutes = _stepMinutes;
    }
    if (minutes > _maxMinutes) {
        minutes = _maxMinutes;
    }
    _minutes = minutes;
}

void TimerView::start() {
    _totalSeconds = _minutes * 60;
    _startMs = millis();
    _phase = Phase::Running;
    _ringsRemaining = 0;  // in case a previous ring was still playing out
    Buzzer::confirm();
}

void TimerView::cancel() {
    _phase = Phase::Setting;
    Buzzer::tap();
}

void TimerView::finish() {
    _phase = Phase::Done;
    if (_beepOnComplete) {
        // First beep fires right away from render(); the rest follow at
        // kRingIntervalMs apart without blocking the render loop.
        _ringsRemaining = kRingCount;
        _nextRingMs = millis();
    } else {
        Buzzer::confirm();
    }
    if (_reportId.length() > 0) {
        publishReport(Report{_reportId, "0"});
    }
}

void TimerView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    if (_phase == Phase::Running) {
        long elapsedSeconds = static_cast<long>(millis() - _startMs) / 1000;
        int remaining = static_cast<int>(_totalSeconds - elapsedSeconds);
        if (remaining <= 0) {
            remaining = 0;
            finish();  // transitions to Phase::Done and publishes the "0" completion report
        }

        int outerR = (cx < cy ? cx : cy) - 10;
        int innerR = outerR - 20;
        float fraction = _totalSeconds > 0 ? static_cast<float>(remaining) / static_cast<float>(_totalSeconds) : 0.f;

        canvas.fillArc(cx, cy, outerR, innerR, 0, 360, kTrackColor);
        if (fraction > 0.f) {
            canvas.fillArc(cx, cy, outerR, innerR, 0, 360.0f * fraction, kAccentColor);
        }

        int mm = remaining / 60;
        int ss = remaining % 60;
        char buf[8];
        snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);

        canvas.setTextColor(WHITE);
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(3);
        canvas.drawString(buf, cx, cy - 10);

        canvas.setTextSize(1);
        canvas.drawString("tap to cancel", cx, cy + 30);
        return;
    }

    if (_phase == Phase::Done) {
        if (_ringsRemaining > 0 && millis() >= _nextRingMs) {
            Buzzer::ring();
            _ringsRemaining--;
            _nextRingMs = millis() + kRingIntervalMs;
        }

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
    canvas.drawString(_name, cx, cy - 60);

    canvas.setTextColor(kAccentColor);
    canvas.setTextSize(4);
    canvas.drawString(String(_minutes) + " min", cx, cy);

    canvas.setTextColor(WHITE);
    canvas.setTextSize(1);
    canvas.drawString("rotate to set, tap to start", cx, cy + 40);
}

namespace {
struct TimerViewRegistrar {
    TimerViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Dial.Node.TimerView",
                                              []() { return std::make_unique<TimerView>(); });
    }
} timerViewRegistrar;
}  // namespace
