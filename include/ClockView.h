#pragma once

#include <Arduino.h>

#include "IView.h"

// Idle/screensaver-style home screen: shows the current time (and date once
// SNTP has synced; see MqttConnection/main.cpp's configTime() call). Takes
// no commandTemplates/reportTemplates - registered like any other view so
// the orchestrator can place it explicitly in deviceConfigurations, and also
// used internally by ViewManager as the auto-shown idle screen after an
// inactivity timeout.
class ClockView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void render(M5Canvas& canvas) override;
};
