#pragma once

#include <Arduino.h>

#include <vector>

#include <riot2/BleTypes.h>

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
// positional order 0/1/2 if `address` isn't set - see resolveReportTemplate()
// in BLEView.cpp):
//   - address "deviceFound"   -> a previously-unseen device started advertising
//   - address "deviceLost"    -> a previously-seen device is no longer present
//   - address "advertisement" -> forwarded for every advertisement received
// Any of the three may be omitted from the configuration; that scenario is
// then simply not reported (the on-screen device list keeps working
// regardless).
//
// Optional per-reportTemplate `parameters` entry `allowedAddresses`: a
// comma-separated list of BLE MAC addresses (matched case-insensitively)
// that THIS report should be restricted to - e.g.
// "AA:BB:CC:DD:EE:FF, 11:22:33:44:55:66" on the "deviceFound" reportTemplate.
// Each of the three reportTemplates has its own independent filter, so e.g.
// "advertisement" reports can be restricted to a short list while
// "deviceFound"/"deviceLost" remain unfiltered. Devices excluded by a given
// report's filter are still discovered/tracked and shown on screen as
// before, they just don't publish that particular report. If a
// reportTemplate has no `allowedAddresses` parameter (or it's blank), no
// filtering is applied for that report, matching the pre-existing behavior.
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
    std::vector<String> _deviceFoundAllowedAddresses;
    std::vector<String> _deviceLostAllowedAddresses;
    std::vector<String> _advertisementAllowedAddresses;

    std::vector<BleDeviceInfo> _devices;
    int _scrollOffset = 0;

    void clampScrollOffset();
    static bool isAddressAllowed(const std::vector<String>& allowedAddresses, const String& address);
};
