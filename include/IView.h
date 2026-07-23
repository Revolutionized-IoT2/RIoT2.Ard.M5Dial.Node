#pragma once

#include <Arduino.h>
#include <M5GFX.h>

#include <functional>

#include "Command.h"
#include "DeviceConfiguration.h"
#include "Report.h"

// Common interface every on-device View implements. The ViewManager carousel
// owns lifecycle/input routing; concrete views (Phase 5) only need to
// override the hooks that are relevant to them - everything except begin()
// and render() has a no-op default.
class IView {
public:
    using ReportCallback = std::function<void(const Report&)>;

    virtual ~IView() = default;

    // One-time setup from the orchestrator-provided configuration for this view.
    virtual void begin(const DeviceConfiguration& config) = 0;

    // Lifecycle: called when the carousel focuses/unfocuses this view.
    virtual void onEnter() {}
    virtual void onExit() {}

    // Input handling while this view is focused. The ViewManager forwards
    // encoder deltas here only while isInteracting() returns true; otherwise
    // encoder rotation navigates the carousel instead.
    virtual void onEncoderChange(int delta) { (void)delta; }
    virtual void onButtonPress() {}
    virtual void onTouch(int x, int y) { (void)x; (void)y; }

    // Whether this view currently wants to consume encoder input itself
    // (e.g. an active "adjust value" submode) instead of the ViewManager
    // using the encoder to move between carousel items.
    virtual bool isInteracting() const { return false; }

    // Apply an inbound command addressed to one of this view's commandTemplates.
    virtual void onCommand(const Command& command) { (void)command; }

    // Draw to the M5Dial's off-screen canvas (called every frame while focused).
    virtual void render(M5Canvas& canvas) = 0;

    // Set by the ViewManager before begin(). Views that produce telemetry
    // call publishReport() to emit a Report for one of their reportTemplates.
    void setReportCallback(ReportCallback callback) { _reportCallback = callback; }

protected:
    void publishReport(const Report& report) {
        if (_reportCallback) {
            _reportCallback(report);
        }
    }

private:
    ReportCallback _reportCallback;
};
