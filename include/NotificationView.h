#pragma once

#include <Arduino.h>

#include "IView.h"

// Transient full-screen notification pushed by an inbound Command (e.g. a
// heads-up status update) - like AlertView, isAlert() makes it take over the
// display immediately when its Command arrives, regardless of what the
// carousel/another view was showing (see ViewManager::onCommand()). Unlike
// AlertView, it doesn't require a deliberate acknowledgement: it
// auto-dismisses back to the carousel after a short timeout on its own
// (wantsExit()), though a tap dismisses it early too.
//
// The Command's `value` supplies what to show: a plain string is used
// directly as the message, or an object with "title"/"message" fields for
// both. Falls back to this view's "title"/"message" deviceParameters (or a
// generic "Notification" title) when the value doesn't specify them. The
// auto-dismiss timeout defaults to 4 seconds, overridable via this view's
// "durationMs" deviceParameter.
class NotificationView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onEnter() override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

    bool isAlert() const override { return true; }
    bool wantsExit() override;

private:
    String _defaultTitle;
    String _defaultMessage;
    String _title;
    String _message;
    unsigned long _durationMs = 4000;
    unsigned long _shownAtMs = 0;
    bool _dismissed = true;  // nothing to show until a command actually arrives
};
