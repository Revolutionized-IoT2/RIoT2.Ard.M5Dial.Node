#include "NotificationView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kNotificationColor = ViewColors::toRGB565(ViewColors::Notification);          // this view's assigned color
const uint16_t kSecondaryColor = ViewColors::toRGB565(ViewColors::NotificationSecondary);  // pale accent for secondary text
constexpr uint16_t kBackground = 0x1082;                                                   // near-black blue-grey
}  // namespace

void NotificationView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void NotificationView::onEnter() {
    _model.show();
    if (_model.soundEnabled()) {
        Buzzer::confirm();
    }
}

void NotificationView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    _model.dismiss();  // tapping dismisses it early
}

void NotificationView::onCommand(const Command& command) {
    _model.onCommand(command);
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
    canvas.drawString(_model.title(), cx, cy);

    if (_model.message().length() > 0) {
        canvas.setTextColor(kSecondaryColor, kBackground);
        canvas.setTextSize(1);
        canvas.drawString(_model.message(), cx, cy + 26);
    }

    if (_model.subHeader().length() > 0) {
        canvas.setTextColor(kSecondaryColor, kBackground);
        canvas.setTextSize(1);
        canvas.drawString(_model.subHeader(), cx, cy + 44);
    }

    // Thin progress bar showing time remaining before auto-dismiss.
    int barWidth = 140;
    int barX = cx - barWidth / 2;
    int barY = canvas.height() - 28;
    canvas.drawRect(barX, barY, barWidth, 6, kSecondaryColor);
    canvas.fillRect(barX + 1, barY + 1, static_cast<int>((barWidth - 2) * _model.remaining()), 4, kNotificationColor);
}

namespace {
struct NotificationViewRegistrar {
    NotificationViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.NotificationView", []() { return std::make_unique<NotificationView>(); },
            []() { return riot2::views::NotificationModel::buildTemplate("RIoT2.Ard.M5Dial.Node.NotificationView"); });
    }
} notificationViewRegistrar;
}  // namespace
