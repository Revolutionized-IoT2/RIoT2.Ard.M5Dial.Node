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

    // Whether this view is doing ongoing background work that needs to keep
    // rendering (e.g. TimerView actively counting down) even without new
    // touch/encoder/button input. While this returns true and the view is
    // focused, the ViewManager suppresses its idle/ClockView timeout - a
    // countdown wouldn't otherwise advance its own completion (buzzer,
    // report) once the idle screen takes over rendering, since that only
    // happens from inside the view's own render().
    virtual bool keepsAwake() const { return false; }

    // Apply an inbound command addressed to one of this view's commandTemplates.
    virtual void onCommand(const Command& command) { (void)command; }

    // Views like AlertView/NotificationView that must interrupt whatever the
    // carousel/another view is currently showing as soon as their command
    // arrives (rather than waiting for the user to dial/tap their way to
    // them) return true here. See ViewManager::onCommand().
    virtual bool isAlert() const { return false; }

    // RFIDView-style views that read tags directly from the node's on-device
    // RFID reader (rather than being driven by an inbound Command) return
    // true here so ViewManager::notifyRfidTagRead() knows to route hardware
    // tag reads to them and, like isAlert(), take over the display as soon
    // as a tag is read.
    virtual bool consumesRfidEvents() const { return false; }

    // Called by ViewManager::notifyRfidTagRead() when the node's on-device
    // RFID reader reads a new tag, for views where consumesRfidEvents() is
    // true. `value` is the tag's UID (hex string).
    virtual void onRfidTagRead(const String& value) { (void)value; }

    // Polled by the ViewManager once per frame while this view is focused;
    // returning true asks the ViewManager to return to the home carousel
    // (e.g. after a transient alert/notification has been acknowledged or
    // has auto-dismissed itself). Most views never need this - the physical
    // button already returns to the carousel unconditionally.
    virtual bool wantsExit() { return false; }

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
