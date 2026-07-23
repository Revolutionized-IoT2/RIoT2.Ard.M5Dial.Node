#pragma once

#include <Arduino.h>

// Node provisioning parameters, persisted in NVS via Preferences.
//
// Phase 2 will add a proper provisioning UX (captive portal / BLE / serial
// command). Until then, optional DEFAULT_* build flags (see platformio.ini)
// seed these values into NVS the first time the device boots with no stored
// config, so Phase 1 connectivity can be exercised without a provisioning
// flow.
struct NodeConfig {
    String id;
    String name;
    String wifiSsid;
    String wifiPassword;
    String mqttServerUrl;
    String mqttUsername;
    String mqttPassword;

    bool isValid() const {
        return id.length() > 0 && wifiSsid.length() > 0 && mqttServerUrl.length() > 0;
    }
};

class NodeConfigStore {
public:
    static NodeConfig load();
    static void save(const NodeConfig& config);
    static void clear();

    // Derives a stable, GUID-shaped default node id from the chip's eFuse
    // MAC, used to prefill the provisioning form. The user can overwrite it.
    static String generateDefaultId();
};
