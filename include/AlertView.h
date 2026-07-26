#pragma once

#include <Arduino.h>

#include "IView.h"

// Transient full-screen alert pushed by an inbound Command (e.g. doorbell,
// alarm). Unlike most views, an AlertView doesn't wait for the user to dial
// or tap their way to it: as soon as a Command addressed to one of its
// commandTemplates arrives, isAlert() tells the ViewManager to immediately
// take over the display (see ViewManager::onCommand()), regardless of what
// the carousel or another focused view was showing. It stays up until
// explicitly acknowledged with a tap (or the physical button, which always
// returns to the carousel) - it never auto-dismisses on its own, since it's
// meant for things that need a deliberate acknowledgement.
//
// The Command's `value` is expected to be an object with "title", "message",
// and "subHeader" fields (e.g. { "title": "Doorbell", "message": "Someone is
// at the front door", "subHeader": "tap to acknowledge" }); a plain string is
// used directly as the message instead. Falls back to generic built-in
// defaults ("Alert" title, empty message, "tap to acknowledge" subHeader)
// when the value doesn't specify them.
class AlertView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onEnter() override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

    bool isAlert() const override { return true; }
    bool wantsExit() override { return _acknowledged; }

private:
    String _title;
    String _message;
    String _subHeader;
    bool _acknowledged = true;  // nothing to show until a command actually arrives
};
