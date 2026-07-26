#include "BleScanner.h"

#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {

// Hex-encodes an arbitrary byte string (e.g. raw BLE manufacturer data) so
// it can be embedded as a plain JSON string value in a Report - mirrors the
// uppercase-hex convention main.cpp::pollRfid() uses for RFID UIDs.
String hexEncode(const std::string& raw) {
    String out;
    out.reserve(raw.size() * 2);
    for (unsigned char b : raw) {
        if (b < 0x10) {
            out += '0';
        }
        out += String(b, HEX);
    }
    out.toUpperCase();
    return out;
}

}  // namespace

// BLEAdvertisedDeviceCallbacks subclass invoked by the BLE stack's own task
// for every advertisement seen (wantDuplicates=true in begin(), so this
// fires repeatedly for the same device, not just the first time). Keeps no
// state of its own - just copies out what it needs and hands it to
// BleScanner::enqueue(), which is the only place that touches shared state,
// under a mutex.
class BleScanner::AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        PendingEvent event;
        event.address = String(advertisedDevice.getAddress().toString().c_str());
        event.name = advertisedDevice.haveName() ? String(advertisedDevice.getName().c_str()) : String();
        event.rssi = advertisedDevice.haveRSSI() ? advertisedDevice.getRSSI() : 0;
        event.manufacturerDataHex =
            advertisedDevice.haveManufacturerData() ? hexEncode(advertisedDevice.getManufacturerData()) : String();
        BleScanner::instance().enqueue(event);
    }
};

BleScanner& BleScanner::instance() {
    static BleScanner scanner;
    return scanner;
}

void BleScanner::begin() {
    if (_began) {
        return;
    }

    _mutex = xSemaphoreCreateMutex();
    _callbacks = new AdvertisedDeviceCallbacks();

    BLEDevice::init("");
    BLEScan* scan = BLEDevice::getScan();
    // wantDuplicates=true so onResult() fires for every advertisement (not
    // just the first sighting of a device) - required to satisfy "forward
    // every BLE advertisement received" (see loop()'s onAdvertisement call).
    scan->setAdvertisedDeviceCallbacks(_callbacks, /*wantDuplicates=*/true, /*shouldParse=*/true);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(99);

    _began = true;
    startScanCycle();
}

void BleScanner::startScanCycle() {
    // Non-blocking continuous scan: start() returns immediately and invokes
    // onScanComplete() once kScanCycleSeconds elapses, which immediately
    // re-arms another cycle - see BLEScan::start()'s scanCompleteCB
    // parameter. is_continue=true so the scan's own internal result cache
    // (unused here - we track devices ourselves) isn't needlessly cleared
    // between cycles.
    BLEDevice::getScan()->start(kScanCycleSeconds, [](BLEScanResults) { BleScanner::onScanComplete(); },
                                 /*is_continue=*/true);
}

void BleScanner::onScanComplete() {
    instance().startScanCycle();
}

void BleScanner::enqueue(const PendingEvent& event) {
    SemaphoreHandle_t mutex = static_cast<SemaphoreHandle_t>(_mutex);
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;  // Main loop is busy; drop this advertisement rather than block the BLE stack's task.
    }
    if (_pending.size() < kMaxPendingEvents) {
        _pending.push_back(event);
    }
    xSemaphoreGive(mutex);
}

BleDeviceInfo* BleScanner::findDevice(const String& address) {
    for (auto& device : _devices) {
        if (device.address == address) {
            return &device;
        }
    }
    return nullptr;
}

void BleScanner::loop() {
    if (!_began) {
        return;
    }

    std::vector<PendingEvent> drained;
    SemaphoreHandle_t mutex = static_cast<SemaphoreHandle_t>(_mutex);
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        drained.swap(_pending);
        xSemaphoreGive(mutex);
    }

    unsigned long now = millis();
    for (const auto& event : drained) {
        BleDeviceInfo* existing = findDevice(event.address);
        if (existing == nullptr) {
            BleDeviceInfo info{event.address, event.name, event.rssi, now};
            _devices.push_back(info);
            if (_onDiscovered) {
                _onDiscovered(info);
            }
        } else {
            existing->name = event.name;
            existing->rssi = event.rssi;
            existing->lastSeenMs = now;
        }

        if (_onAdvertisement) {
            _onAdvertisement(BleAdvertisement{event.address, event.name, event.rssi, event.manufacturerDataHex});
        }
    }

    for (size_t i = 0; i < _devices.size();) {
        if (now - _devices[i].lastSeenMs >= kDeviceTimeoutMs) {
            String address = _devices[i].address;
            _devices.erase(_devices.begin() + i);
            if (_onLost) {
                _onLost(address);
            }
        } else {
            ++i;
        }
    }
}
