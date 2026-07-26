#pragma once

#include <Arduino.h>

#include "IView.h"

// Transient full-screen view triggered by an inbound Command as soon as an
// RFID tag is read (e.g. by an external reader device that publishes the
// scanned tag's value to one of this view's commandTemplates) - like
// NotificationView, isAlert() makes it take over the display immediately
// when its Command arrives, regardless of what the carousel/another view
// was showing (see ViewManager::onCommand()). It confirms the read with a
// beep as soon as it's shown, then auto-dismisses back to the carousel on
// its own after a short timeout (wantsExit()), though a tap dismisses it
// early too.
//
// The Command's `value` supplies the tag value to display: a plain
// string/number is used directly, or an object with a "value" field
// (falling back to "tag"/"uid"). Falls back to this view's "title" /
// "placeholder" deviceParameters (or generic defaults) when nothing has
// been read yet. The auto-dismiss timeout defaults to 4 seconds,
// overridable via this view's "durationMs" deviceParameter.
class RFIDView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onEnter() override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

    bool isAlert() const override { return true; }
    bool wantsExit() override;

private:
    String _title;
    String _placeholder;
    String _tagValue;
    unsigned long _durationMs = 4000;
    unsigned long _shownAtMs = 0;
    bool _dismissed = true;  // nothing to show until a command actually arrives
};
