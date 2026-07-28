#include "BLEView.h"

#include <ArduinoJson.h>

#include <memory>

#include "ViewColors.h"
#include "ViewFactory.h"

namespace {
const uint16_t kPrimaryColor = ViewColors::toRGB565(ViewColors::BLE);            // this view's assigned color
const uint16_t kSecondaryColor = ViewColors::toRGB565(ViewColors::BLESecondary);  // pale companion color

// Resolves which reportTemplate (if any) should be used for one of BLEView's
// three report scenarios: prefers an exact match on `address`, falling back
// to the given positional index so a minimal configuration that just lists
// up to three reportTemplates in order (without bothering to set `address`)
// still works, mirroring ButtonView/ValueView's address-matching convention.
// Returns a pointer into `templates` (not a copy) so callers can also read
// that specific reportTemplate's own `parameters` (e.g. its `allowedAddresses`
// filter); nullptr if no match/fallback exists.
const ReportTemplate* resolveReportTemplate(const std::vector<ReportTemplate>& templates, const String& address,
                                             size_t fallbackIndex) {
    for (const auto& tmpl : templates) {
        if (tmpl.address == address) {
            return &tmpl;
        }
    }
    return fallbackIndex < templates.size() ? &templates[fallbackIndex] : nullptr;
}

// Builds the JSON report value for a discovered/updated device. Uses
// ArduinoJson's serializer (rather than hand-built string concatenation) so
// the advertised `name` - untrusted data broadcast over the air by whatever
// nearby device - is always properly JSON-escaped rather than risking
// malformed/injected report payloads.
String deviceJson(const String& address, const String& name, int rssi) {
    JsonDocument doc;
    doc["address"] = address;
    doc["name"] = name;
    doc["rssi"] = rssi;
    String out;
    serializeJson(doc, out);
    return out;
}

String advertisementJson(const BleAdvertisement& advertisement) {
    JsonDocument doc;
    doc["address"] = advertisement.address;
    doc["name"] = advertisement.name;
    doc["rssi"] = advertisement.rssi;
    doc["manufacturerData"] = advertisement.manufacturerDataHex;
    String out;
    serializeJson(doc, out);
    return out;
}

// Splits the `allowedAddresses` deviceParameter (comma-separated MAC
// addresses, e.g. "AA:BB:CC:DD:EE:FF, 11:22:33:44:55:66") into a list of
// trimmed, non-empty tokens. An empty/blank input yields an empty vector.
std::vector<String> splitAddressList(const String& raw) {
    std::vector<String> result;
    int start = 0;
    while (start <= static_cast<int>(raw.length())) {
        int comma = raw.indexOf(',', start);
        String token = (comma == -1) ? raw.substring(start) : raw.substring(start, comma);
        token.trim();
        if (token.length() > 0) {
            result.push_back(token);
        }
        if (comma == -1) {
            break;
        }
        start = comma + 1;
    }
    return result;
}

}  // namespace

void BLEView::begin(const DeviceConfiguration& config) {
    _header = findParameter(config.deviceParameters, "header", "Nearby BLE");

    const ReportTemplate* deviceFoundTmpl = resolveReportTemplate(config.reportTemplates, "deviceFound", 0);
    const ReportTemplate* deviceLostTmpl = resolveReportTemplate(config.reportTemplates, "deviceLost", 1);
    const ReportTemplate* advertisementTmpl = resolveReportTemplate(config.reportTemplates, "advertisement", 2);

    _deviceFoundReportId = deviceFoundTmpl ? deviceFoundTmpl->id : String();
    _deviceLostReportId = deviceLostTmpl ? deviceLostTmpl->id : String();
    _advertisementReportId = advertisementTmpl ? advertisementTmpl->id : String();

    // Each report has its own independent `allowedAddresses` filter, read
    // from that specific reportTemplate's `parameters` (not deviceParameters).
    _deviceFoundAllowedAddresses =
        deviceFoundTmpl ? splitAddressList(findParameter(deviceFoundTmpl->parameters, "allowedAddresses", ""))
                        : std::vector<String>();
    _deviceLostAllowedAddresses =
        deviceLostTmpl ? splitAddressList(findParameter(deviceLostTmpl->parameters, "allowedAddresses", ""))
                       : std::vector<String>();
    _advertisementAllowedAddresses =
        advertisementTmpl ? splitAddressList(findParameter(advertisementTmpl->parameters, "allowedAddresses", ""))
                          : std::vector<String>();

    _devices.clear();
    _scrollOffset = 0;
}

// A report's filter is restricted to `allowedAddresses` when it's non-empty;
// an empty list (parameter absent/blank on that reportTemplate) means "no
// filtering", matching the pre-existing behavior of reporting every
// discovered device/advertisement.
bool BLEView::isAddressAllowed(const std::vector<String>& allowedAddresses, const String& address) {
    if (allowedAddresses.empty()) {
        return true;
    }
    for (const auto& allowed : allowedAddresses) {
        if (allowed.equalsIgnoreCase(address)) {
            return true;
        }
    }
    return false;
}

void BLEView::clampScrollOffset() {
    int maxOffset = static_cast<int>(_devices.size()) - kVisibleRows;
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
    return static_cast<int>(_devices.size()) > kVisibleRows;
}

void BLEView::onBleDeviceDiscovered(const BleDeviceInfo& device) {
    _devices.push_back(device);
    if (_deviceFoundReportId.length() > 0 && isAddressAllowed(_deviceFoundAllowedAddresses, device.address)) {
        publishReport(Report{_deviceFoundReportId, deviceJson(device.address, device.name, device.rssi)});
    }
}

void BLEView::onBleDeviceLost(const String& address) {
    for (size_t i = 0; i < _devices.size(); ++i) {
        if (_devices[i].address == address) {
            _devices.erase(_devices.begin() + i);
            break;
        }
    }
    clampScrollOffset();

    if (_deviceLostReportId.length() > 0 && isAddressAllowed(_deviceLostAllowedAddresses, address)) {
        // `address` is always a colon-separated hex MAC (from BLEAddress::toString()),
        // so it's safe to embed directly as a quoted JSON string literal.
        publishReport(Report{_deviceLostReportId, String("\"") + address + "\""});
    }
}

void BLEView::onBleAdvertisement(const BleAdvertisement& advertisement) {
    if (_advertisementReportId.length() > 0 &&
        isAddressAllowed(_advertisementAllowedAddresses, advertisement.address)) {
        publishReport(Report{_advertisementReportId, advertisementJson(advertisement)});
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
    canvas.drawString(_header, cx, 31);

    if (_devices.empty()) {
        canvas.setTextColor(kSecondaryColor, BLACK);
        canvas.setTextSize(1);
        canvas.drawString("No devices found", cx, canvas.height() / 2);
        return;
    }

    int rowHeight = 34;
    int firstRowY = 76;
    int visibleCount = min(kVisibleRows, static_cast<int>(_devices.size()) - _scrollOffset);

    for (int i = 0; i < visibleCount; ++i) {
        const BleDeviceInfo& device = _devices[_scrollOffset + i];
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
    canvas.drawString(String(_devices.size()) + (_devices.size() == 1 ? " device" : " devices"), cx,
                       canvas.height() - 18);
}

namespace {
struct BLEViewRegistrar {
    BLEViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Dial.Node.BLEView",
                                              []() { return std::make_unique<BLEView>(); });
    }
} bleViewRegistrar;
}  // namespace
