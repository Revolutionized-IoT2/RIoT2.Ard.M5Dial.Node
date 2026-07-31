#include "RFIDView.h"

#include <memory>

#include <riot2/Uuid.h>

#include "Buzzer.h"
#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kRFIDColor = ViewColors::toRGB565(ViewColors::RFID);            // this view's assigned color
const uint16_t kSecondaryColor = ViewColors::toRGB565(ViewColors::RFIDSecondary);  // pale accent for secondary text
constexpr uint16_t kBackground = 0x1082;                                       // near-black blue-grey

DeviceConfiguration buildRFIDViewTemplate() {
    DeviceConfiguration config;
    config.id = riot2::newId();
    config.name = "RFID View";
    config.classFullName = "RIoT2.Ard.M5Dial.Node.RFIDView";
    config.deviceParameters = {{"title", "RFID Tag"}, {"durationMs", "4000"}, {"subHeader", "scan a tag"}};

    ReportTemplate report;
    report.id = riot2::newId();
    report.type = "1";
    report.name = "RFID Tag";
    config.reportTemplates.push_back(report);
    return config;
}
}  // namespace

void RFIDView::begin(const DeviceConfiguration& config) {
    _title = findParameter(config.deviceParameters, "title", "RFID Tag");
    _reportId = config.reportTemplates.empty() ? "" : config.reportTemplates[0].id;
    _tagValue = "";

    String durationParam = findParameter(config.deviceParameters, "durationMs", "");
    _durationMs = durationParam.length() > 0 ? static_cast<unsigned long>(durationParam.toInt()) : 4000;
    if (_durationMs == 0) {
        _durationMs = 4000;
    }

    _dismissed = true;
}

void RFIDView::onEnter() {
    _dismissed = false;
    _shownAtMs = millis();
    Buzzer::confirm();
}

void RFIDView::onTouch(int x, int y) {
    (void)x;
    (void)y;
    _dismissed = true;  // tapping dismisses it early
}

void RFIDView::onRfidTagRead(const String& value) {
    _tagValue = value;
    if (_reportId.length() > 0) {
        publishReport(Report{_reportId, String("\"") + value + "\""});
    }
}

bool RFIDView::wantsExit() {
    return _dismissed || (millis() - _shownAtMs) >= _durationMs;
}


void RFIDView::render(M5Canvas& canvas) {
    canvas.fillScreen(kBackground);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    // Contactless-card glyph above the text: a rounded card outline with a
    // couple of concentric "signal" arcs fanning off its top-right corner,
    // matching the usual RFID/NFC iconography.
    int cardW = 44;
    int cardH = 30;
    int cardY = cy - 74;
    canvas.drawRoundRect(cx - cardW / 2, cardY - cardH / 2, cardW, cardH, 5, kRFIDColor);
    canvas.drawRoundRect(cx - cardW / 2 + 1, cardY - cardH / 2 + 1, cardW - 2, cardH - 2, 4, kRFIDColor);
    canvas.fillRoundRect(cx - cardW / 2 + 5, cardY - 3, 12, 8, 2, kRFIDColor);

    int waveCx = cx + cardW / 2 - 2;
    int waveCy = cardY - cardH / 2 - 2;
    for (int i = 0; i < 3; ++i) {
        int radius = 6 + i * 6;
        canvas.drawArc(waveCx, waveCy, radius, radius + 1, 200.0f, 340.0f, kRFIDColor);
    }

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE, kBackground);
    canvas.setTextSize(2);
    canvas.drawString(_title, cx, cy - 20);

    canvas.setTextColor(kRFIDColor, kBackground);
    canvas.setTextSize(_tagValue.length() > 12 ? 1 : 2);
    canvas.drawString(_tagValue.length() > 0 ? _tagValue : "--", cx, cy + 8);

    // Thin progress bar showing time remaining before auto-dismiss.
    unsigned long elapsed = millis() - _shownAtMs;
    float remaining = _durationMs == 0 ? 0.f : 1.f - static_cast<float>(elapsed) / static_cast<float>(_durationMs);
    if (remaining < 0.f) remaining = 0.f;
    if (remaining > 1.f) remaining = 1.f;

    int barWidth = 140;
    int barX = cx - barWidth / 2;
    int barY = canvas.height() - 28;
    canvas.drawRect(barX, barY, barWidth, 6, kSecondaryColor);
    canvas.fillRect(barX + 1, barY + 1, static_cast<int>((barWidth - 2) * remaining), 4, kRFIDColor);
}

namespace {
struct RFIDViewRegistrar {
    RFIDViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Dial.Node.RFIDView",
                                              []() { return std::make_unique<RFIDView>(); }, buildRFIDViewTemplate);
    }
} rfidViewRegistrar;
}  // namespace
