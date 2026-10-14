#pragma once

#include <Arduino.h>

#include <vector>

#include <riot2/BleTypes.h>
#include <riot2/views/BleModel.h>

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
// Renders the shared riot2::views::BleModel; the device list, the
// "deviceFound"/"deviceLost"/"advertisement" reports and their per-report
// `allowedAddresses` filters are documented there. Rotate the dial to scroll
// once there are more devices than fit on screen.
class BLEView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void onEncoderChange(int delta) override;
    bool isInteracting() const override;
    void render(M5Canvas& canvas) override;

    bool consumesBleEvents() const override { return true; }
    void onBleSnapshot(const std::vector<BleDeviceInfo>& devices) override;
    void onBleDeviceDiscovered(const BleDeviceInfo& device) override;
    void onBleDeviceLost(const String& address) override;
    void onBleAdvertisement(const BleAdvertisement& advertisement) override;

private:
    static constexpr int kVisibleRows = 4;

    riot2::views::BleModel _model;
    int _scrollOffset = 0;

    void clampScrollOffset();
};