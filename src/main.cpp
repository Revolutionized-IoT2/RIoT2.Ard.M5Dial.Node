#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Dial.h>
#include <WiFi.h>
#include <time.h>

#include "Buzzer.h"
#include "Command.h"
#include "MqttConnection.h"
#include "NodeConfig.h"
#include "OrchestratorClient.h"
#include "OtaUpdater.h"
#include "ProvisioningPortal.h"
#include "Report.h"
#include "ViewManager.h"
#include "WifiConnection.h"

namespace {

// Long-press on the M5Dial's physical button (BtnA) to clear NodeConfig and
// re-enter provisioning mode on next boot.
constexpr unsigned long kFactoryResetHoldMs = 5000;

// Medium-press to toggle the on-device diagnostics overlay (Wi-Fi/MQTT
// status, signal strength, free heap). Shorter than the factory-reset hold.
constexpr unsigned long kDiagnosticsHoldMs = 1500;

// Power management: dim after kDimTimeoutMs of no input, fully sleep the
// panel after kSleepTimeoutMs. Any input restores full brightness/wakes it;
// the very first input that wakes the panel from sleep is swallowed (not
// forwarded to the active view), matching typical smart-home dial UX.
constexpr unsigned long kDimTimeoutMs = 15000;
constexpr unsigned long kSleepTimeoutMs = 60000;
constexpr uint8_t kDimBrightness = 15;

// Reserved MQTT command id (RIoT2.Ard.M5Dial.Node-specific extension, not
// part of RIoT2.Core's Command contract) that triggers an OTA update instead
// of being routed to a view: { "id": "system.ota", "value": "<firmware url>" }.
const char* const kOtaCommandId = "system.ota";

enum class AppMode { Provisioning, Normal };

AppMode mode = AppMode::Normal;
NodeConfig config;
WifiConnection wifi;
MqttConnection mqtt;
ProvisioningPortal provisioning;
OrchestratorClient orchestratorClient;
ViewManager viewManager;
M5Canvas canvas(&M5Dial.Display);

WifiState lastWifiState = WifiState::Disconnected;
MqttState lastMqttState = MqttState::Disconnected;
bool factoryResetTriggered = false;
bool diagnosticsHoldTriggered = false;
unsigned long buttonPressStartMs = 0;
bool showDiagnostics = false;
long lastEncoderPosition = 0;

// M5Dial's rotary encoder reports multiple raw quadrature counts per
// physical click/detent (confirmed by observation: one click moved the
// carousel highlight by 4 items before this was accounted for). Raw ticks
// are accumulated in encoderRemainder and only converted into whole
// "clicks" (what ViewManager/views actually expect as a +/-1 step) once a
// full detent's worth has been seen.
constexpr int kEncoderCountsPerClick = 4;
int encoderRemainder = 0;

uint8_t fullBrightness = 128;
bool dimmed = false;
bool asleep = false;
unsigned long lastActivityMs = 0;

void showStatus(const String& line1, const String& line2 = "") {
    M5Dial.Display.fillScreen(BLACK);
    M5Dial.Display.setTextColor(WHITE);
    M5Dial.Display.setTextDatum(middle_center);

    M5Dial.Display.setTextSize(2);
    M5Dial.Display.drawString(line1, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 - 10);

    if (line2.length()) {
        M5Dial.Display.setTextSize(1);
        M5Dial.Display.drawString(line2, M5Dial.Display.width() / 2, M5Dial.Display.height() / 2 + 15);
    }
}

// Restores full brightness / wakes the panel from sleep. Returns true if the
// panel had to be woken from a full sleep (caller should swallow the input
// that triggered this rather than acting on it).
bool wakeDisplay() {
    bool wasAsleep = asleep;
    if (asleep) {
        M5Dial.Display.wakeup();
        asleep = false;
    }
    if (dimmed) {
        M5Dial.Display.setBrightness(fullBrightness);
        dimmed = false;
    }
    lastActivityMs = millis();
    return wasAsleep;
}

void updatePowerManagement() {
    unsigned long idleMs = millis() - lastActivityMs;
    if (!asleep && idleMs >= kSleepTimeoutMs) {
        M5Dial.Display.sleep();
        asleep = true;
    } else if (!asleep && !dimmed && idleMs >= kDimTimeoutMs) {
        M5Dial.Display.setBrightness(kDimBrightness);
        dimmed = true;
    }
}

void renderDiagnostics(M5Canvas& target) {
    target.fillScreen(BLACK);
    target.setTextColor(WHITE);
    target.setTextDatum(middle_center);
    target.setTextSize(2);
    target.drawString("Diagnostics", target.width() / 2, 30);

    target.setTextDatum(middle_left);
    target.setTextSize(1);
    int x = 20;
    int y = 70;
    const int lineHeight = 22;

    target.drawString("Node: " + config.name, x, y);
    y += lineHeight;
    target.drawString(String("WiFi: ") + (wifi.isConnected() ? "connected" : "connecting..."), x, y);
    y += lineHeight;
    if (wifi.isConnected()) {
        target.drawString("SSID: " + WiFi.SSID(), x, y);
        y += lineHeight;
        target.drawString("RSSI: " + String(WiFi.RSSI()) + " dBm", x, y);
        y += lineHeight;
    }
    target.drawString(String("MQTT: ") + (mqtt.isConnected() ? "connected" : "connecting..."), x, y);
    y += lineHeight;
    target.drawString("Free heap: " + String(ESP.getFreeHeap() / 1024) + " KB", x, y);
    y += lineHeight;
    target.drawString("Uptime: " + String(millis() / 1000) + " s", x, y);
    y += lineHeight;

    target.setTextDatum(middle_center);
    target.drawString("hold to close", target.width() / 2, target.height() - 20);
}

void handleCommand(const String& topic, const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("[Command] Failed to parse %s: %s\n", topic.c_str(), error.c_str());
        return;
    }

    String id = doc["id"] | "";
    if (id.length() == 0) {
        Serial.printf("[Command] %s missing id, ignoring\n", topic.c_str());
        return;
    }

    if (id == kOtaCommandId) {
        String url = doc["value"] | "";
        Serial.printf("[OTA] Update requested: %s\n", url.c_str());
        Buzzer::confirm();
        showStatus("Updating...", "Do not power off");
        if (!OtaUpdater::performUpdate(url)) {
            Buzzer::error();
            showStatus("Update failed", "See serial log");
        }
        return;
    }

    Command command{id, doc["value"]};
    viewManager.onCommand(command.id, command);
}

void handleReport(const Report& report) {
    Serial.printf("[Report] id=%s value=%s\n", report.id.c_str(), report.value.c_str());
    mqtt.publishReport(report);
}

void handleOrchestratorOnline(const String& topic, const String& payload) {
    // Corrected handshake: riot2/orchestrator/online carries no payload (or
    // an ignorable one). The node simply re-announces itself; the
    // orchestrator replies with the apiBaseUrl on
    // riot2/node/{id}/configuration (see handleConfigurationMessage).
    Serial.println("[Orchestrator] Orchestrator online, re-announcing node");
    mqtt.publishOnline();
}

void handleConfigurationMessage(const String& topic, const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("[Orchestrator] Failed to parse configuration message: %s\n", error.c_str());
        return;
    }

    String apiBaseUrl = doc["apiBaseUrl"] | "";
    if (apiBaseUrl.length() == 0) {
        Serial.println("[Orchestrator] configuration message missing apiBaseUrl");
        return;
    }

    orchestratorClient.requestConfiguration(apiBaseUrl, config.id);
}

void handleConfigurationUpdated(const NodeConfiguration& nodeConfiguration) {
    for (const auto& device : nodeConfiguration.deviceConfigurations) {
        Serial.printf("[Orchestrator]   view id=%s name=%s classFullName=%s (%u commands, %u reports)\n",
                      device.id.c_str(), device.name.c_str(), device.classFullName.c_str(),
                      static_cast<unsigned>(device.commandTemplates.size()),
                      static_cast<unsigned>(device.reportTemplates.size()));
    }
    viewManager.rebuild(nodeConfiguration);
}

void factoryReset() {
    Serial.println("[Config] Factory reset requested, clearing NodeConfig");
    Buzzer::error();
    showStatus("Resetting...", "Clearing config");
    mqtt.publishOfflineAndDisconnect();
    NodeConfigStore::clear();
    delay(500);
    ESP.restart();
}

}  // namespace

void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);

    config = NodeConfigStore::load();

    if (!config.isValid()) {
        Serial.println("[Config] No valid NodeConfig in NVS. Starting provisioning portal.");
        mode = AppMode::Provisioning;
        provisioning.begin();
        showStatus("Setup needed", "WiFi: " + provisioning.apSsid());
        return;
    }

    Serial.printf("[Config] Loaded node id=%s name=%s\n", config.id.c_str(), config.name.c_str());
    showStatus("Starting...", config.name);

    fullBrightness = M5Dial.Display.getBrightness();
    lastActivityMs = millis();

    canvas.createSprite(M5Dial.Display.width(), M5Dial.Display.height());

    wifi.begin(config.wifiSsid, config.wifiPassword);

    // SNTP sync so Report.timeStamp is a real Unix epoch; opportunistic, runs
    // once Wi-Fi comes up. Reports published before the first sync completes
    // will carry a small/incorrect timestamp - acceptable for now.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    viewManager.onReport(handleReport);
    orchestratorClient.onConfigurationUpdated(handleConfigurationUpdated);

    mqtt.onCommand(handleCommand);
    mqtt.onOrchestratorOnline(handleOrchestratorOnline);
    mqtt.onConfigurationMessage(handleConfigurationMessage);
    mqtt.begin(config);
}

void loop() {
    M5Dial.update();

    if (mode == AppMode::Provisioning) {
        provisioning.loop();
        return;
    }

    wifi.loop();
    mqtt.loop();

    bool statusChanged = wifi.state() != lastWifiState || mqtt.state() != lastMqttState;
    if (statusChanged) {
        lastWifiState = wifi.state();
        lastMqttState = mqtt.state();
    }

    if (!factoryResetTriggered && M5Dial.BtnA.pressedFor(kFactoryResetHoldMs)) {
        factoryResetTriggered = true;
        factoryReset();
        return;
    }

    if (!diagnosticsHoldTriggered && M5Dial.BtnA.pressedFor(kDiagnosticsHoldMs)) {
        diagnosticsHoldTriggered = true;
        showDiagnostics = !showDiagnostics;
        Buzzer::confirm();
    }

    if (M5Dial.BtnA.wasPressed()) {
        buttonPressStartMs = millis();
    }

    // A plain tap (button released before the diagnostics-hold threshold,
    // and not already consumed by that hold gesture) is forwarded to
    // ViewManager, whose onButtonPress() always returns to the home
    // carousel when a view is focused (a no-op otherwise) - the physical
    // button is reserved exclusively for that gesture; views themselves are
    // driven entirely by touch and the bezel (encoder).
    bool buttonPressed = false;
    if (M5Dial.BtnA.wasReleased()) {
        if (!diagnosticsHoldTriggered && (millis() - buttonPressStartMs) < kDiagnosticsHoldMs) {
            buttonPressed = true;
        }
        diagnosticsHoldTriggered = false;
    }

    long encoderPosition = M5Dial.Encoder.read();
    int rawEncoderDelta = static_cast<int>(encoderPosition - lastEncoderPosition);
    lastEncoderPosition = encoderPosition;

    int encoderClicks = 0;
    if (rawEncoderDelta != 0) {
        // Discard any leftover partial-click "remainder" from the previous
        // direction as soon as the wheel reverses. Without this, a reversal
        // has to first pay down the old direction's leftover ticks before a
        // full click accumulates in the new direction, which feels like it
        // takes two clicks to register a direction change.
        if ((rawEncoderDelta > 0 && encoderRemainder < 0) || (rawEncoderDelta < 0 && encoderRemainder > 0)) {
            encoderRemainder = 0;
        }

        encoderRemainder += rawEncoderDelta;
        encoderClicks = encoderRemainder / kEncoderCountsPerClick;
        encoderRemainder -= encoderClicks * kEncoderCountsPerClick;
    }

    bool touched = false;
    int touchX = 0;
    int touchY = 0;
    if (M5Dial.Touch.getCount()) {
        auto detail = M5Dial.Touch.getDetail(0);
        if (detail.wasPressed()) {
            touched = true;
            touchX = detail.x;
            touchY = detail.y;
        }
    }

    bool hadInput = encoderClicks != 0 || buttonPressed || touched;
    if (hadInput && wakeDisplay()) {
        hadInput = false;  // first tap after sleep just wakes the screen
    }

    updatePowerManagement();

    if (showDiagnostics) {
        renderDiagnostics(canvas);
        canvas.pushSprite(0, 0);
        return;
    }

    if (viewManager.hasViews()) {
        if (hadInput) {
            // Forward one onEncoderChange() call per physical click, so a
            // fast spin that accumulates several clicks between loop()
            // iterations still moves the highlight/value that many steps
            // (callers only look at the sign of delta, not its magnitude).
            for (int i = 0; i < abs(encoderClicks); ++i) {
                viewManager.onEncoderChange(encoderClicks > 0 ? 1 : -1);
            }
            if (buttonPressed) {
                viewManager.onButtonPress();
            }
            if (touched) {
                viewManager.onTouch(touchX, touchY);
            }
        }

        viewManager.render(canvas);
        canvas.pushSprite(0, 0);
    } else if (statusChanged) {
        String wifiLine = wifi.isConnected() ? "WiFi: connected" : "WiFi: connecting...";
        String mqttLine = mqtt.isConnected() ? "MQTT: connected" : "MQTT: connecting...";
        showStatus(wifiLine, mqttLine);
    }
}