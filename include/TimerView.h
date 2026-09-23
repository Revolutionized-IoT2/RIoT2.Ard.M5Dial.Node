#pragma once

#include <Arduino.h>

#include "IView.h"

// Countdown timer. While idle ("Setting"), rotate the dial to choose a
// duration in whole minutes and tap to start; while running, the view shows
// the remaining time (with an arc as visual feedback, mirroring
// PercentageView/SliderView) and a tap cancels back to Setting. When the
// countdown reaches zero it shows a "Time's up!" screen until tapped, which
// dismisses it back to Setting. Duration is configured via deviceParameters:
// "defaultMinutes" (default 5), "stepMinutes" (default 1), "maxMinutes"
// (default 60) - the dial is clamped to [stepMinutes, maxMinutes]. An
// optional boolean deviceParameter "beepOnComplete" (default false), when
// "true", plays a repeating "egg timer ring" (a handful of Buzzer::ring()
// beeps spaced out over loop() calls, non-blocking) instead of the usual
// single confirm chirp once the countdown finishes.
//
// Publishes a single Report with the remaining time ("0") only once the
// countdown completes - it does not report every second while running.
// Execution runs from loop(), including while another view, popup, diagnostics
// or the idle screen is shown. Rendering never completes the timer or sends
// reports. Reconfiguration destroys the old timer without a completion report.
// An inbound Command addressed to
// this view's commandTemplate presets the duration (in minutes) remotely,
// but only while idle in Setting (never interrupts a running countdown).
class TimerView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onTouch(int x, int y) override;
    void onEncoderChange(int delta) override;
    void onCommand(const Command& command) override;
    void loop() override;
    bool isInteracting() const override { return _phase == Phase::Setting; }
    bool keepsAwake() const override { return _phase == Phase::Running; }
    void render(M5Canvas& canvas) override;

private:
    enum class Phase { Setting, Running, Done };

    String _commandId;
    String _reportId;
    String _name;
    int _stepMinutes = 1;
    int _maxMinutes = 60;
    int _minutes = 5;
    bool _beepOnComplete = false;
    Phase _phase = Phase::Setting;
    unsigned long _startMs = 0;
    int _totalSeconds = 0;
    int _ringsRemaining = 0;
    unsigned long _lastRingMs = 0;

    int remainingSeconds() const;
    void start();
    void cancel();
    void finish();
};
