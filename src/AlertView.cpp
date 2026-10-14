#include "AlertView.h"

#include <memory>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kAlertColor = ViewColors::toRGB565(ViewColors::Alert);                    // this view's assigned color
const uint16_t kAlertSecondaryColor = ViewColors::toRGB565(ViewColors::AlertSecondary);  // pale accent for secondary text
}  // namespace

void AlertView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void AlertView::onEnter() {
    _model.show();
    if (_model.soundEnabled()) {
        Buzzer::alert();
    }
}

void AlertView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    if (_model.acknowledge()) {
        Buzzer::confirm();
    }
}

void AlertView::onCommand(const Command& command) {
    _model.onCommand(command);
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
    canvas.drawString(_model.title(), cx, cy);

    canvas.setTextColor(kAlertSecondaryColor, kAlertColor);
    if (_model.message().length() > 0) {
        canvas.setTextSize(1);
        canvas.drawString(_model.message(), cx, cy + 26);
    }

    canvas.drawString(_model.subHeader(), cx, canvas.height() - 22);
}

namespace {
struct AlertViewRegistrar {
    AlertViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.AlertView", []() { return std::make_unique<AlertView>(); },
            []() { return riot2::views::AlertModel::buildTemplate("RIoT2.Ard.M5Dial.Node.AlertView"); });
    }
} alertViewRegistrar;
}  // namespace
