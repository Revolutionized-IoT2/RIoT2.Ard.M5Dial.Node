#pragma once

#include <Arduino.h>

#include <riot2/views/NotificationModel.h>

#include "IView.h"

// Transient full-screen notification pushed by an inbound Command (e.g. a
// heads-up status update) - like AlertView, isAlert() makes it take over the
// display immediately when its Command arrives, regardless of what the
// carousel/another view was showing (see ViewManager::onCommand()). Unlike
// AlertView, it doesn't require a deliberate acknowledgement: it
// auto-dismisses back to the carousel after a short timeout on its own
// (wantsExit()), though a tap dismisses it early too.
//
// The Command's `value` is parsed by the shared
// riot2::views::NotificationModel: an object with "title", "message", and
// "subHeader" fields (e.g. { "title": "Doorbell", "message": "Someone is at
// the front door", "subHeader": "tap to acknowledge" }), or a plain string
// used directly as the message. Falls back to generic built-in defaults
// ("Notification" title, empty message/subHeader) when the value doesn't
// specify them. The auto-dismiss timeout defaults to 4 seconds, overridable
// via this view's "durationMs" deviceParameter.
//
// The Command's `value` object may also include a "soundEnabled" field
// (default false) controlling whether a notification ping (Buzzer::confirm())
// plays when that particular command takes over the display - e.g.
// { "title": "...", "message": "...", "soundEnabled": true }.
class NotificationView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onEnter() override;
    void onTouch(int x, int y) override;
    void onCommand(const Command& command) override;
    void render(M5Canvas& canvas) override;

    bool isAlert() const override { return true; }
    bool wantsExit() override { return _model.isExpired(); }

private:
    riot2::views::NotificationModel _model;
};
