#pragma once

#include <Arduino.h>

#include <riot2/views/AlertModel.h>

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
// The Command's `value` is parsed by the shared riot2::views::AlertModel: an
// object with "title", "message", and "subHeader" fields (e.g. { "title":
// "Doorbell", "message": "Someone is at the front door", "subHeader": "tap to
// acknowledge" }), or a plain string used directly as the message. Falls back
// to generic built-in defaults ("Alert" title, empty message, "tap to
// acknowledge" subHeader) when the value doesn't specify them.
//
// The Command's `value` object may also include a "soundEnabled" field
// (default false) controlling whether an alert sound (Buzzer::alert()) plays
// when that particular command takes over the display - e.g. { "title": "...",
// "message": "...", "soundEnabled": true }.
class AlertView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onEnter() override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

    bool isAlert() const override { return true; }
    bool wantsExit() override { return _model.isAcknowledged(); }

private:
    riot2::views::AlertModel _model;
};
