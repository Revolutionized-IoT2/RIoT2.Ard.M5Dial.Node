#pragma once

#include <Arduino.h>

#include "IView.h"

// Idle/screensaver-style home screen: shows the current time (and date once
// SNTP has synced; see MqttConnection/main.cpp's configTime() call). Takes
// no commandTemplates/reportTemplates - registered like any other view so
// the orchestrator can place it explicitly in deviceConfigurations, and also
// used internally by ViewManager as the auto-shown idle screen after an
// inactivity timeout.
//
// Supports an optional "timezone" deviceParameter: a POSIX TZ string (e.g.
// "EET-2EEST,M3.5.0/3,M10.5.0/4" for Helsinki, with full DST support) applied
// via setenv("TZ", ...)/tzset() so localtime_r() in render() shows local
// time instead of UTC. If absent, time is shown in UTC (main.cpp's
// configTime() call has no gmtOffset/DST applied).
class ClockView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void render(M5Canvas& canvas) override;
};
