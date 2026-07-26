#include "NotificationView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kNotificationColor = ViewColors::toRGB565(ViewColors::Notification);          // this view's assigned color
const uint16_t kSecondaryColor = ViewColors::toRGB565(ViewColors::NotificationSecondary);  // pale accent for secondary text
constexpr uint16_t kBackground = 0x1082;                                                   // near-black blue-grey

// Pulls a human-readable title/message out of a Command's loosely-typed
// value: a plain string is used as the message directly, an object looks
// for "title"/"message" (falling back to "text") keys, anything else (or a
// missing key) falls back to the supplied default.
String extractField(const JsonVariantConst& value, const char* key, const String& fallback) {
    if (value.is<JsonObjectConst>()) {
        JsonObjectConst obj = value.as<JsonObjectConst>();
        if (obj[key].is<const char*>()) {
            return obj[key].as<String>();
        }
    }
    return fallback;
}

String extractMessage(const JsonVariantConst& value, const String& fallback) {
    if (value.is<const char*>()) {
        return value.as<String>();
    }
    return extractField(value, "message", extractField(value, "text", fallback));
}

// Reads an optional "soundEnabled" field out of the Command's `value` object.
// Accepts a real JSON boolean (expected) or a "true"/"false" string, so it
// tolerates either representation from the orchestrator.
bool extractSoundEnabled(const JsonVariantConst& value, bool fallback) {
    if (value.is<JsonObjectConst>()) {
        JsonVariantConst v = value.as<JsonObjectConst>()["soundEnabled"];
        if (v.is<bool>()) {
            return v.as<bool>();
        }
        if (v.is<const char*>()) {
            return String(v.as<const char*>()).equalsIgnoreCase("true");
        }
    }
    return fallback;
}
}  // namespace

void NotificationView::begin(const DeviceConfiguration& config) {
    _title = "Notification";
    _message = "";
    _subHeader = "";

    String durationParam = findParameter(config.deviceParameters, "durationMs", "");
    _durationMs = durationParam.length() > 0 ? static_cast<unsigned long>(durationParam.toInt()) : 4000;
    if (_durationMs == 0) {
        _durationMs = 4000;
    }

    _soundEnabled = false;
    _dismissed = true;
}

void NotificationView::onEnter() {
    _dismissed = false;
    _shownAtMs = millis();
    if (_soundEnabled) {
        Buzzer::confirm();
    }
}

void NotificationView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    _dismissed = true;  // tapping dismisses it early
}

void NotificationView::onCommand(const Command& command) {
    _title = extractField(command.value, "title", "Notification");
    _message = extractMessage(command.value, "");
    _subHeader = extractField(command.value, "subHeader", "");
    _soundEnabled = extractSoundEnabled(command.value, false);
}

bool NotificationView::wantsExit() {
    return _dismissed || (millis() - _shownAtMs) >= _durationMs;
}

void NotificationView::render(M5Canvas& canvas) {
    canvas.fillScreen(kBackground);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    // Bell glyph above the text, matching the carousel icon.
    int r = 30;
    int by = cy - 62;
    canvas.fillTriangle(cx - r * 0.5f, by + r * 0.15f, cx + r * 0.5f, by + r * 0.15f, cx, by - r * 0.6f,
                        kNotificationColor);
    canvas.fillCircle(cx, by - r * 0.05f, r * 0.42f, kNotificationColor);
    canvas.fillRoundRect(cx - r * 0.55f, by + r * 0.05f, r * 1.1f, r * 0.16f, r * 0.08f, kNotificationColor);
    canvas.fillCircle(cx, by + r * 0.4f, r * 0.14f, kNotificationColor);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE, kBackground);
    canvas.setTextSize(2);
    canvas.drawString(_title, cx, cy);

    if (_message.length() > 0) {
        canvas.setTextColor(kSecondaryColor, kBackground);
        canvas.setTextSize(1);
        canvas.drawString(_message, cx, cy + 26);
    }

    if (_subHeader.length() > 0) {
        canvas.setTextColor(kSecondaryColor, kBackground);
        canvas.setTextSize(1);
        canvas.drawString(_subHeader, cx, cy + 44);
    }

    // Thin progress bar showing time remaining before auto-dismiss.
    unsigned long elapsed = millis() - _shownAtMs;
    float remaining = _durationMs == 0 ? 0.f : 1.f - static_cast<float>(elapsed) / static_cast<float>(_durationMs);
    if (remaining < 0.f) remaining = 0.f;
    if (remaining > 1.f) remaining = 1.f;

    int barWidth = 140;
    int barX = cx - barWidth / 2;
    int barY = canvas.height() - 28;
    canvas.drawRect(barX, barY, barWidth, 6, kSecondaryColor);
    canvas.fillRect(barX + 1, barY + 1, static_cast<int>((barWidth - 2) * remaining), 4, kNotificationColor);
}

namespace {
struct NotificationViewRegistrar {
    NotificationViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.NotificationView",
                                              []() { return std::make_unique<NotificationView>(); });
    }
} notificationViewRegistrar;
}  // namespace
