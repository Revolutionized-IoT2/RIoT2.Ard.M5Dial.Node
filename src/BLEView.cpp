#include "BLEView.h"

#include <memory>

#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kPrimaryColor = ViewColors::toRGB565(ViewColors::BLE);            // this view's assigned color
const uint16_t kSecondaryColor = ViewColors::toRGB565(ViewColors::BLESecondary);  // pale companion color
}  // namespace

void BLEView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
    _scrollOffset = 0;
}

void BLEView::clampScrollOffset() {
    int maxOffset = static_cast<int>(_model.devices().size()) - kVisibleRows;
    if (maxOffset < 0) {
        maxOffset = 0;
    }
    if (_scrollOffset > maxOffset) {
        _scrollOffset = maxOffset;
    }
    if (_scrollOffset < 0) {
        _scrollOffset = 0;
    }
}

void BLEView::onEncoderChange(int delta) {
    if (!isInteracting()) {
        return;
    }
    _scrollOffset += (delta > 0 ? 1 : -1);
    clampScrollOffset();
}

bool BLEView::isInteracting() const {
    return static_cast<int>(_model.devices().size()) > kVisibleRows;
}

void BLEView::onBleSnapshot(const std::vector<BleDeviceInfo>& devices) {
    _model.onSnapshot(devices);
    clampScrollOffset();
}

void BLEView::onBleDeviceDiscovered(const BleDeviceInfo& device) {
    Report report;
    if (_model.onDeviceDiscovered(device, report)) {
        publishReport(report);
    }
}

void BLEView::onBleDeviceLost(const String& address) {
    Report report;
    bool reported = _model.onDeviceLost(address, report);
    clampScrollOffset();
    if (reported) {
        publishReport(report);
    }
}

void BLEView::onBleAdvertisement(const BleAdvertisement& advertisement) {
    Report report;
    if (_model.onAdvertisement(advertisement, report)) {
        publishReport(report);
    }
}

void BLEView::render(M5Canvas& canvas) {
    canvas.fillScreen(BLACK);

    int cx = canvas.width() / 2;

    // Header banner: Primary color as a filled background (like ButtonView's
    // active button fill) rather than as text-on-black, since BLE's assigned
    // Primary (#263238) is too dark to read as text directly on the black
    // canvas background.
    canvas.fillRoundRect(20, 14, canvas.width() - 40, 34, 10, kPrimaryColor);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(WHITE, kPrimaryColor);
    canvas.setTextSize(2);
    canvas.drawString(_model.header(), cx, 31);

    const std::vector<BleDeviceInfo>& devices = _model.devices();
    if (devices.empty()) {
        canvas.setTextColor(kSecondaryColor, BLACK);
        canvas.setTextSize(1);
        canvas.drawString("No devices found", cx, canvas.height() / 2);
        return;
    }

    int rowHeight = 34;
    int firstRowY = 76;
    int visibleCount = min(kVisibleRows, static_cast<int>(devices.size()) - _scrollOffset);

    for (int i = 0; i < visibleCount; ++i) {
        const BleDeviceInfo& device = devices[_scrollOffset + i];
        int rowY = firstRowY + i * rowHeight;

        String label = device.name.length() > 0 ? device.name : device.address;
        canvas.setTextDatum(middle_left);
        canvas.setTextColor(WHITE, BLACK);
        canvas.setTextSize(1);
        canvas.drawString(label, 26, rowY);

        canvas.setTextDatum(middle_right);
        canvas.setTextColor(kSecondaryColor, BLACK);
        canvas.drawString(String(device.rssi) + " dBm", canvas.width() - 20, rowY);
    }

    canvas.setTextDatum(middle_center);
    canvas.setTextColor(kSecondaryColor, BLACK);
    canvas.setTextSize(1);
    canvas.drawString(String(devices.size()) + (devices.size() == 1 ? " device" : " devices"), cx,
                       canvas.height() - 18);
}

namespace {
struct BLEViewRegistrar {
    BLEViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Dial.Node.BLEView", []() { return std::make_unique<BLEView>(); },
            []() { return riot2::views::BleModel::buildTemplate("RIoT2.Ard.M5Dial.Node.BLEView"); });
    }
} bleViewRegistrar;
}  // namespace