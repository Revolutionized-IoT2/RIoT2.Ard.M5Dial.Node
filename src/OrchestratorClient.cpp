#include "OrchestratorClient.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace {

String buildConfigurationUrl(const String& apiBaseUrl, const String& nodeId) {
    String base = apiBaseUrl;
    if (!base.endsWith("/")) {
        base += "/";
    }
    return base + "api/Nodes/" + nodeId + "/configuration";
}

// Shared by deviceConfigurations (Views) and peripheralConfigurations (Grove
// port peripherals) - both use an identical shape (id/name/classFullName/
// commandTemplates/reportTemplates/deviceParameters), just resolved through
// a different factory (ViewFactory vs PeripheralFactory) by the rest of the
// app.
DeviceConfiguration parseDeviceConfiguration(JsonObject deviceJson) {
    DeviceConfiguration device;
    device.id = deviceJson["id"] | "";
    device.name = deviceJson["name"] | "";
    device.classFullName = deviceJson["classFullName"] | "";

    for (JsonObject cmdJson : deviceJson["commandTemplates"].as<JsonArray>()) {
        CommandTemplate cmd;
        cmd.id = cmdJson["id"] | "";
        cmd.type = cmdJson["type"] | "";
        cmd.name = cmdJson["name"] | "";
        cmd.address = cmdJson["address"] | "";
        cmd.valueType = cmdJson["valueType"] | 0;
        cmd.model = cmdJson["model"] | false;
        device.commandTemplates.push_back(cmd);
    }

    for (JsonObject reportJson : deviceJson["reportTemplates"].as<JsonArray>()) {
        ReportTemplate report;
        report.id = reportJson["id"] | "";
        report.type = reportJson["type"] | "";
        report.name = reportJson["name"] | "";
        report.address = reportJson["address"] | "";
        for (JsonPair kv : reportJson["parameters"].as<JsonObject>()) {
            report.parameters.push_back({String(kv.key().c_str()), kv.value().as<String>()});
        }
        device.reportTemplates.push_back(report);
    }

    for (JsonPair kv : deviceJson["deviceParameters"].as<JsonObject>()) {
        device.deviceParameters.push_back({String(kv.key().c_str()), kv.value().as<String>()});
    }

    return device;
}

}  // namespace

// Out-of-line definition required pre-C++17 whenever a static constexpr
// member is ODR-used (see repo memory notes on ProvisioningPortal::kDnsPort).
constexpr uint32_t OrchestratorClient::kHttpTimeoutMs;

bool OrchestratorClient::requestConfiguration(const String& apiBaseUrl, const String& nodeId) {
    String url = buildConfigurationUrl(apiBaseUrl, nodeId);
    Serial.printf("[Orchestrator] Fetching configuration from %s\n", url.c_str());

    HTTPClient http;
    http.setTimeout(kHttpTimeoutMs);

    // The orchestrator may be reachable over plain HTTP (typical on a local
    // network) or HTTPS with a self-signed/local cert; no CA pinning yet.
    WiFiClientSecure secureClient;
    WiFiClient plainClient;
    bool began = url.startsWith("https://") ? (secureClient.setInsecure(), http.begin(secureClient, url))
                                             : http.begin(plainClient, url);

    if (!began) {
        Serial.println("[Orchestrator] Failed to start HTTP request");
        return false;
    }

    int statusCode = http.GET();
    if (statusCode != HTTP_CODE_OK) {
        Serial.printf("[Orchestrator] GET failed, status=%d\n", statusCode);
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    NodeConfiguration parsed;
    if (!parseConfiguration(body, parsed)) {
        Serial.println("[Orchestrator] Failed to parse configuration JSON");
        return false;
    }

    _current = parsed;
    Serial.printf("[Orchestrator] Configuration updated: %u device configuration(s)\n",
                  static_cast<unsigned>(_current.deviceConfigurations.size()));

    if (_callback) {
        _callback(_current);
    }

    return true;
}

bool OrchestratorClient::parseConfiguration(const String& json, NodeConfiguration& out) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.printf("[Orchestrator] JSON parse error: %s\n", error.c_str());
        return false;
    }

    out.name = doc["name"] | "";
    out.id = doc["id"] | "";
    out.deviceConfigurations.clear();
    out.peripheralConfigurations.clear();

    for (JsonObject deviceJson : doc["deviceConfigurations"].as<JsonArray>()) {
        out.deviceConfigurations.push_back(parseDeviceConfiguration(deviceJson));
    }

    // Grove-port peripherals (see IPeripheral/PeripheralManager) - an
    // optional top-level array, same shape as deviceConfigurations. Missing
    // entirely from older/unaware orchestrator responses is fine: iterating
    // an absent/invalid JsonArray simply yields zero entries.
    for (JsonObject peripheralJson : doc["peripheralConfigurations"].as<JsonArray>()) {
        out.peripheralConfigurations.push_back(parseDeviceConfiguration(peripheralJson));
    }

    return true;
}
