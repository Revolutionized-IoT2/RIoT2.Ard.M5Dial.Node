#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Encoder.h>
#include <M5Dial.h>
#include <WiFi.h>
#include <time.h>

#include "BleScanner.h"
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

// M5Dial.Encoder (bundled M5Dial lib) decodes quadrature in software via the
// PJRC Encoder library's pin-change ISRs. On fast bezel spins the edge rate
// can outrun how quickly those ISRs get serviced (WiFi/BT stack activity,
// flash access, rendering, etc. all momentarily hold off interrupts on this
// single-core-busy ESP32-S3), so a transition is occasionally missed
// entirely and that detent's rotation is silently lost ("skips a beat" -
// the menu doesn't move for that click). ESP32's PCNT peripheral counts
// quadrature pulses in dedicated hardware with no CPU/ISR involvement per
// edge, so it can't drop counts under interrupt latency the way the
// software decoder can. `dialEncoder` replaces `M5Dial.Encoder` for actual
// position reads; M5Dial.begin() is still told enableEncoder=false so the
// bundled PJRC decoder never attaches its own interrupts on the same pins.
ESP32Encoder dialEncoder;

uint8_t fullBrightness = 128;
bool dimmed = false;
bool asleep = false;
unsigned long lastActivityMs = 0;

// Debounces the node's on-device RFID reader (M5Dial.Rfid, a built-in
// MFRC522 - see M5Dial.begin()'s enableRFID flag): ignores repeat reads of
// the same tag within this window so leaving a card resting on/near the
// reader doesn't keep re-triggering RFIDView every loop().
constexpr unsigned long kRfidRepeatSuppressMs = 3000;
String lastRfidUid;
unsigned long lastRfidReadMs = 0;

// The on-device RFID reader is only powered up/polled once the active
// configuration actually includes a view that consumes tag reads (e.g.
// RFIDView) - see updateRfidActivation(). rfidInitialized guards against
// calling M5Dial.Rfid.begin() more than once; rfidActive gates pollRfid().
bool rfidInitialized = false;
bool rfidActive = false;

// The node's on-device BLE radio is likewise only powered on once the
// active configuration actually includes a view that consumes BLE scan
// events (e.g. BLEView) - see handleConfigurationUpdated(). Once started,
// BleScanner scans continuously in the background (see BleScanner.h); there
// is no corresponding "turn back off" path, mirroring the RFID reader.
bool bleActive = false;

// Set by handleConfigurationMessage() when riot2/node/{id}/configuration
// arrives, and consumed once from the top-level loop() rather than being
// acted on immediately inside the MQTT callback. requestConfiguration()
// performs a blocking HTTP GET (up to several seconds) - running it
// synchronously inside PubSubClient's own callback dispatch (mqtt.loop() ->
// _client.loop() -> this callback) was observed to starve the client's
// keepalive/socket processing for that whole duration, causing the MQTT
// connection itself to drop (and even the HTTP GET's own socket to
// read-time-out: "GET failed, status=-11") right as the fetch completed.
// That, in turn, triggered reconnect -> republish online -> orchestrator
// resends configuration -> blocking fetch -> disconnect again: a
// self-sustaining loop every couple of minutes that reset the whole
// UI/carousel and looked like the node was rebooting. Deferring the actual
// fetch to loop() (outside of _client.loop()'s call stack) avoids
// re-entering/blocking PubSubClient's own processing.
bool pendingConfigFetch = false;
String pendingApiBaseUrl;

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

    // See the pendingConfigFetch comment above: don't call
    // orchestratorClient.requestConfiguration() (blocking HTTP GET) from
    // here directly - just record the request and let loop() perform it
    // outside of PubSubClient's own callback dispatch.
    pendingApiBaseUrl = apiBaseUrl;
    pendingConfigFetch = true;
}

void handleConfigurationUpdated(const NodeConfiguration& nodeConfiguration) {
    for (const auto& device : nodeConfiguration.deviceConfigurations) {
        Serial.printf("[Orchestrator]   view id=%s name=%s classFullName=%s (%u commands, %u reports)\n",
                      device.id.c_str(), device.name.c_str(), device.classFullName.c_str(),
                      static_cast<unsigned>(device.commandTemplates.size()),
                      static_cast<unsigned>(device.reportTemplates.size()));
    }
    viewManager.rebuild(nodeConfiguration);

    // Only power up the on-device RFID reader if this configuration actually
    // has a view that wants tag reads (e.g. RFIDView) - it stays off
    // otherwise, rather than being enabled unconditionally at boot.
    rfidActive = viewManager.hasRfidConsumer();
    if (rfidActive && !rfidInitialized) {
        Serial.println("[RFID] Configuration includes an RFID-consuming view, enabling on-device reader");
        M5Dial.Rfid.begin();
        rfidInitialized = true;
    }

    // Same gating for the on-device BLE radio: only enable it if this
    // configuration actually has a view that wants BLE scan events (e.g.
    // BLEView, see BleScanner.h).
    if (viewManager.hasBleConsumer() && !bleActive) {
        Serial.println("[BLE] Configuration includes a BLE-consuming view, enabling on-device BLE scanner");
        BleScanner::instance().begin();
        bleActive = true;
    }
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

// Polls the node's on-device RFID reader; when a new tag is read (and it
// isn't a debounced repeat of the same tag - see kRfidRepeatSuppressMs),
// forwards its UID to ViewManager::notifyRfidTagRead(), which routes it to
// RFIDView (IView::consumesRfidEvents()) regardless of what's currently
// focused/shown, exactly like an inbound command does for AlertView /
// NotificationView.
void pollRfid() {
    if (!M5Dial.Rfid.PICC_IsNewCardPresent() || !M5Dial.Rfid.PICC_ReadCardSerial()) {
        return;
    }

    String uid;
    for (byte i = 0; i < M5Dial.Rfid.uid.size; i++) {
        if (M5Dial.Rfid.uid.uidByte[i] < 0x10) {
            uid += "0";
        }
        uid += String(M5Dial.Rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    M5Dial.Rfid.PICC_HaltA();

    unsigned long now = millis();
    if (uid == lastRfidUid && (now - lastRfidReadMs) < kRfidRepeatSuppressMs) {
        return;
    }
    lastRfidUid = uid;
    lastRfidReadMs = now;

    Serial.printf("[RFID] Tag read: %s\n", uid.c_str());
    viewManager.notifyRfidTagRead(uid);
}

}  // namespace

void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    // enableEncoder=false: skip M5Dial's bundled software (PJRC) quadrature
    // decoder entirely - dialEncoder (hardware PCNT, set up below) reads the
    // same DIAL_ENCODER_PIN_A/B pins without its missed-edge-under-load risk.
    M5Dial.begin(cfg, false, false);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    dialEncoder.attachFullQuad(DIAL_ENCODER_PIN_A, DIAL_ENCODER_PIN_B);
    dialEncoder.setCount(0);

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

    // 8bpp (RGB332) instead of the default 16bpp (RGB565) halves the
    // off-screen canvas from ~115KB to ~58KB. This M5Stamp-S3 module has no
    // PSRAM, and with WiFi + the BLE controller (needed by BLEView) both
    // active, free heap can drop to ~10KB - too little for M5GFX's PNG
    // decoder (its zlib inflate window alone needs ~32KB), so icons
    // silently fail to draw. The color-precision loss (8 levels of
    // red/green, 4 of blue) is an acceptable trade-off for view icons/text,
    // which are mostly solid vivid colors rather than smooth gradients.
    // canvas.setColorDepth(8);
    canvas.createSprite(M5Dial.Display.width(), M5Dial.Display.height());

    wifi.begin(config.wifiSsid, config.wifiPassword);

    // SNTP sync so Report.timeStamp is a real Unix epoch; opportunistic, runs
    // once Wi-Fi comes up. Reports published before the first sync completes
    // will carry a small/incorrect timestamp - acceptable for now.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    viewManager.onReport(handleReport);
    orchestratorClient.onConfigurationUpdated(handleConfigurationUpdated);

    BleScanner::instance().onDeviceDiscovered(
        [](const BleDeviceInfo& device) { viewManager.notifyBleDeviceDiscovered(device); });
    BleScanner::instance().onDeviceLost([](const String& address) { viewManager.notifyBleDeviceLost(address); });
    BleScanner::instance().onAdvertisement(
        [](const BleAdvertisement& advertisement) { viewManager.notifyBleAdvertisement(advertisement); });

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

    // Runs the (blocking) configuration fetch requested by
    // handleConfigurationMessage(), outside of mqtt.loop()'s own callback
    // dispatch - see the pendingConfigFetch comment above.
    if (pendingConfigFetch) {
        pendingConfigFetch = false;
        orchestratorClient.requestConfiguration(pendingApiBaseUrl, config.id);
    }

    if (rfidActive) {
        pollRfid();
    }

    if (bleActive) {
        BleScanner::instance().loop();
    }

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

    long encoderPosition = dialEncoder.getCount();
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