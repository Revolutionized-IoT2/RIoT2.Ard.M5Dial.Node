#include "AlertView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kAlertColor = ViewColors::toRGB565(ViewColors::Alert);                    // this view's assigned color
const uint16_t kAlertSecondaryColor = ViewColors::toRGB565(ViewColors::AlertSecondary);  // pale accent for secondary text

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
}  // namespace

void AlertView::begin(const DeviceConfiguration& config) {
    _defaultTitle = findParameter(config.deviceParameters, "title", "Alert");
    _defaultMessage = findParameter(config.deviceParameters, "message", "");
    _title = _defaultTitle;
    _message = _defaultMessage;
    _acknowledged = true;
}

void AlertView::onEnter() {
    _acknowledged = false;
    Buzzer::error();
}

void AlertView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    if (_acknowledged) {
        return;
    }
    _acknowledged = true;
    Buzzer::confirm();
}

void AlertView::onCommand(const Command& command) {
    _title = extractField(command.value, "title", _defaultTitle);
    _message = extractMessage(command.value, _defaultMessage);
}

void AlertView::render(M5Canvas& canvas) {
    canvas.fillScreen(kAlertColor);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    // Warning triangle glyph above the text, matching the carousel icon.
    int r = 34;
    int ty = cy - 62;
    canvas.fillTriangle(cx, ty - r * 0.7f, cx - r * 0.65f, ty + r * 0.55f, cx + r * 0.65f, ty + r * 0.55f, WHITE);
    canvas.fillTriangle(cx, ty - r * 0.7f + r * 0.22f, cx - r * 0.65f + r * 0.14f, ty + r * 0.55f - r * 0.14f,
                        cx + r * 0.65f - r * 0.14f, ty + r * 0.55f - r * 0.14f, kAlertColor);
    canvas.fillCircle(cx, ty + r * 0.28f, r * 0.08f, kAlertColor);
    canvas.fillRect(cx - r * 0.06f, ty - r * 0.15f, r * 0.12f, r * 0.3f, kAlertColor);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE, kAlertColor);
    canvas.setTextSize(2);
    canvas.drawString(_title, cx, cy);

    canvas.setTextColor(kAlertSecondaryColor, kAlertColor);
    if (_message.length() > 0) {
        canvas.setTextSize(1);
        canvas.drawString(_message, cx, cy + 26);
    }

    canvas.drawString("tap to acknowledge", cx, canvas.height() - 22);
}

namespace {
struct AlertViewRegistrar {
    AlertViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.AlertView",
                                              []() { return std::make_unique<AlertView>(); });
    }
} alertViewRegistrar;
}  // namespace
