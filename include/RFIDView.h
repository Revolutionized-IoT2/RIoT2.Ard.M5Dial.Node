#pragma once

#include <Arduino.h>

#include "IView.h"

// Transient full-screen view triggered directly by the node's on-device
// RFID reader (M5Dial's built-in MFRC522, see M5Dial.Rfid) as soon as a tag
// is read - main.cpp polls the reader every loop() and forwards new reads to
// ViewManager::notifyRfidTagRead(), which routes them to whichever entry has
// consumesRfidEvents() (this view). Like AlertView/NotificationView,
// ViewManager takes over the display for it immediately, regardless of what
// the carousel/another view was showing. It confirms the read with a beep as
// soon as it's shown, publishes a Report (this view's first reportTemplate)
// with the tag's UID, then auto-dismisses back to the carousel on its own
// after a short timeout (wantsExit()), though a tap dismisses it early too.
//
// This view's "title" deviceParameter customizes the heading (defaults to
// "RFID Tag"); "durationMs" overrides the 4 second auto-dismiss timeout.
class RFIDView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onEnter() override;
    void onTouch(int x, int y) override;
    void onRfidTagRead(const String& value) override;
    void render(M5Canvas& canvas) override;

    bool isAlert() const override { return true; }
    bool consumesRfidEvents() const override { return true; }
    bool wantsExit() override;

private:
    String _title;
    String _reportId;
    String _tagValue;
    unsigned long _durationMs = 4000;
    unsigned long _shownAtMs = 0;
    bool _dismissed = true;  // nothing to show until a tag is actually read
};

