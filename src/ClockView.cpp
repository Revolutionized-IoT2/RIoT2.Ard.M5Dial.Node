#include "ClockView.h"

#include <memory>
#include <time.h>

#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
// time(nullptr) returns small values (seconds since boot-ish) before SNTP
// has synced; treat anything before 2020-01-01 as "not yet synced".
constexpr time_t kMinValidEpoch = 1577836800;
const uint16_t kAccentColor = ViewColors::toRGB565(ViewColors::Clock);           // this view's assigned color
const uint16_t kSubtleColor = ViewColors::toRGB565(ViewColors::ClockSecondary);  // pale accent for date/status text
}  // namespace

void ClockView::begin(const DeviceConfiguration& config) {
    (void)config;
}

void ClockView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int cx = canvas.width() / 2;
    int cy = canvas.height() / 2;

    // Thin themed ring around the clock face, matching this view's carousel icon color.
    int radius = (cx < cy ? cx : cy) - 12;
    canvas.drawCircle(cx, cy, radius, kAccentColor);

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE);

    time_t now = time(nullptr);
    struct tm timeInfo;
    if (now >= kMinValidEpoch && localtime_r(&now, &timeInfo) != nullptr) {
        char timeBuf[9];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
        canvas.setTextSize(3);
        canvas.drawString(timeBuf, cx, cy - 10);

        char dateBuf[16];
        snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", timeInfo.tm_year + 1900, timeInfo.tm_mon + 1,
                  timeInfo.tm_mday);
        canvas.setTextColor(kSubtleColor);
        canvas.setTextSize(1);
        canvas.drawString(dateBuf, cx, cy + 25);
    } else {
        canvas.setTextSize(2);
        canvas.drawString("--:--:--", cx, cy - 10);
        canvas.setTextColor(kSubtleColor);
        canvas.setTextSize(1);
        canvas.drawString("waiting for time sync", cx, cy + 25);
    }
}

namespace {
struct ClockViewRegistrar {
    ClockViewRegistrar() {
        ViewFactory::instance().registerView("RIoT2.Ard.M5Dial.Node.ClockView",
                                              []() { return std::make_unique<ClockView>(); });
    }
} clockViewRegistrar;
}  // namespace
