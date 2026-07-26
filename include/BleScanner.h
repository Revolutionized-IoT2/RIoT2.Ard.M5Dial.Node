#pragma once

#include <Arduino.h>

#include <functional>
#include <vector>

#include "BleTypes.h"

// Wraps the M5Dial's on-chip BLE radio (the ESP32 Arduino core's bundled
// BLE library - BLEDevice/BLEScan/BLEAdvertisedDeviceCallbacks, Bluedroid
// backend) to continuously discover nearby BLE advertisers in the
// background. Like the on-device RFID reader (see main.cpp's
// rfidActive/M5Dial.Rfid pattern), the BLE radio is only powered on if the
// active configuration actually includes a view that wants BLE data
// (BLEView, see ViewManager::hasBleConsumer()) - it stays off otherwise,
// rather than being enabled unconditionally at boot.
//
// The underlying BLE stack invokes its scan-result callback from its own
// FreeRTOS task, not the Arduino loop() task, so results are handed off
// through a small mutex-guarded pending queue and only turned into
// onDeviceDiscovered()/onDeviceLost()/onAdvertisement() callbacks (and
// _devices list updates) from inside loop() - keeping all app-visible state
// single-threaded, consistent with the rest of this codebase (MQTT
// publish, view rendering, etc. all happen on the main loop() task).
class BleScanner {
public:
    using DeviceEventCallback = std::function<void(const BleDeviceInfo&)>;
    using DeviceLostCallback = std::function<void(const String& address)>;
    using AdvertisementCallback = std::function<void(const BleAdvertisement&)>;

    static BleScanner& instance();

    // Initializes the BLE stack and starts continuous background scanning.
    // Safe to call more than once (no-op after the first call).
    void begin();

    // Call once per loop() iteration while BLE is active: drains scan
    // events queued by the BLE stack's own task and prunes devices not
    // seen for kDeviceTimeoutMs (firing onDeviceLost() for each).
    void loop();

    bool isActive() const { return _began; }

    void onDeviceDiscovered(DeviceEventCallback callback) { _onDiscovered = std::move(callback); }
    void onDeviceLost(DeviceLostCallback callback) { _onLost = std::move(callback); }
    void onAdvertisement(AdvertisementCallback callback) { _onAdvertisement = std::move(callback); }

    // "No longer present" after this many milliseconds without a new
    // advertisement from a given address.
    static constexpr unsigned long kDeviceTimeoutMs = 60000;

private:
    // Restarted continuously (see BleScanner.cpp) - the scan-complete
    // callback immediately re-arms another cycle, so scanning effectively
    // never stops once begin() is called, without blocking loop().
    static constexpr uint32_t kScanCycleSeconds = 30;
    static constexpr size_t kMaxPendingEvents = 64;

    struct PendingEvent {
        String address;
        String name;
        int rssi = 0;
        String manufacturerDataHex;
    };

    class AdvertisedDeviceCallbacks;

    BleScanner() = default;

    void startScanCycle();
    void enqueue(const PendingEvent& event);
    BleDeviceInfo* findDevice(const String& address);

    static void onScanComplete();

    bool _began = false;
    void* _mutex = nullptr;  // SemaphoreHandle_t, opaque here to avoid pulling FreeRTOS headers into this .h
    std::vector<PendingEvent> _pending;
    std::vector<BleDeviceInfo> _devices;

    DeviceEventCallback _onDiscovered;
    DeviceLostCallback _onLost;
    AdvertisementCallback _onAdvertisement;

    AdvertisedDeviceCallbacks* _callbacks = nullptr;
};
