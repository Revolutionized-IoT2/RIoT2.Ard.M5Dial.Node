#pragma once

#include <Arduino.h>

#include <vector>

#include "BleTypes.h"
#include "IView.h"

// Read-only carousel view that lists nearby BLE advertisers discovered by
// the node's on-device BLE radio (see BleScanner.h) - populated via the
// consumesBleEvents() hooks below, which ViewManager routes to this view
// regardless of whether it's currently focused (same pattern as RFIDView's
// on-device tag reads), since the underlying BLE scan runs continuously in
// the background once BLEView is present in the configuration (see
// ViewManager::hasBleConsumer(), checked in main.cpp). Unlike RFIDView this
// is a normal (non-alert) carousel entry - receiving scan events never
// takes over the display.
//
// Reports are published for three distinct scenarios, each addressed by a
// separate reportTemplate matched by its `address` field (falling back to
// positional order 0/1/2 if `address` isn't set - see resolveReportId() in
// BLEView.cpp):
//   - address "deviceFound"   -> a previously-unseen device started advertising
//   - address "deviceLost"    -> a previously-seen device is no longer present
//   - address "advertisement" -> forwarded for every advertisement received
// Any of the three may be omitted from the configuration; that scenario is
// then simply not reported (the on-screen device list keeps working
// regardless).
class BLEView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onEncoderChange(int delta) override;
    bool isInteracting() const override;
    void render(M5Canvas& canvas) override;

    bool consumesBleEvents() const override { return true; }
    void onBleDeviceDiscovered(const BleDeviceInfo& device) override;
    void onBleDeviceLost(const String& address) override;
    void onBleAdvertisement(const BleAdvertisement& advertisement) override;

private:
    static constexpr int kVisibleRows = 4;

    String _header;
    String _deviceFoundReportId;
    String _deviceLostReportId;
    String _advertisementReportId;

    std::vector<BleDeviceInfo> _devices;
    int _scrollOffset = 0;

    void clampScrollOffset();
};
