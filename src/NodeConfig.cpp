#include "NodeConfig.h"

#include <Preferences.h>

namespace {
constexpr const char* kNamespace = "riot2node";
}

NodeConfig NodeConfigStore::load() {
    Preferences prefs;
    prefs.begin(kNamespace, true);

    NodeConfig config;
    config.id = prefs.getString("id", "");
    config.name = prefs.getString("name", "");
    config.wifiSsid = prefs.getString("ssid", "");
    config.wifiPassword = prefs.getString("pass", "");
    config.mqttServerUrl = prefs.getString("mqttUrl", "");
    config.mqttUsername = prefs.getString("mqttUser", "");
    config.mqttPassword = prefs.getString("mqttPass", "");
    prefs.end();

#if defined(DEFAULT_NODE_ID)
    // First boot with nothing stored yet: seed from build-time defaults so
    // Phase 1 connectivity can be tested before Phase 2's provisioning UX
    // exists.
    if (config.id.length() == 0) {
        config.id = DEFAULT_NODE_ID;
#if defined(DEFAULT_NODE_NAME)
        config.name = DEFAULT_NODE_NAME;
#endif
        config.wifiSsid = DEFAULT_WIFI_SSID;
        config.wifiPassword = DEFAULT_WIFI_PASSWORD;
        config.mqttServerUrl = DEFAULT_MQTT_SERVER_URL;
        config.mqttUsername = DEFAULT_MQTT_USERNAME;
        config.mqttPassword = DEFAULT_MQTT_PASSWORD;
        save(config);
    }
#endif

    if (config.name.length() == 0) {
        config.name = "M5Dial Node";
    }

    return config;
}

void NodeConfigStore::save(const NodeConfig& config) {
    Preferences prefs;
    prefs.begin(kNamespace, false);
    prefs.putString("id", config.id);
    prefs.putString("name", config.name);
    prefs.putString("ssid", config.wifiSsid);
    prefs.putString("pass", config.wifiPassword);
    prefs.putString("mqttUrl", config.mqttServerUrl);
    prefs.putString("mqttUser", config.mqttUsername);
    prefs.putString("mqttPass", config.mqttPassword);
    prefs.end();
}

void NodeConfigStore::clear() {
    Preferences prefs;
    prefs.begin(kNamespace, false);
    prefs.clear();
    prefs.end();
}

String NodeConfigStore::generateDefaultId() {
    uint64_t mac = ESP.getEfuseMac();
    char buf[37];
    snprintf(buf, sizeof(buf), "00000000-0000-4000-8000-%012llX",
             static_cast<unsigned long long>(mac & 0xFFFFFFFFFFFFULL));
    return String(buf);
}
