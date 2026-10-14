#pragma once

#include <Arduino.h>

#include <riot2/views/ClockModel.h>

#include "IView.h"

// Idle/screensaver-style home screen: shows the current time (and date once
// SNTP has synced; see MqttConnection/main.cpp's configTime() call). Takes
// no commandTemplates/reportTemplates - registered like any other view so
// the orchestrator can place it explicitly in deviceConfigurations, and also
// used internally by ViewManager as the auto-shown idle screen after an
// inactivity timeout.
//
// Supports an optional "timezone" deviceParameter (see
// riot2::views::ClockModel): a POSIX TZ string (e.g.
// "EET-2EEST,M3.5.0/3,M10.5.0/4" for Helsinki, with full DST support) so
// render() shows local time instead of UTC.
class ClockView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void render(M5Canvas& canvas) override;

private:
    riot2::views::ClockModel _model;
};
