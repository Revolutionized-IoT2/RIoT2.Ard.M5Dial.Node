#include <Arduino.h>
#include <M5Dial.h>

#include <riot2/NodeRuntime.h>

#include "BoardPeripherals.h"
#include "Buzzer.h"
#include "DialInput.h"
#include "DiagnosticsScreen.h"
#include "DisplayPower.h"
#include "Manifest.h"
#include "RfidReader.h"
#include "StatusScreen.h"
#include "ViewFactory.h"
#include "ViewManager.h"

// Board setup, the carousel and the dial's inputs. The node lifecycle (Wi-Fi, provisioning, MQTT,
// configuration, OTA, peripherals, BLE) is riot2::NodeRuntime in RIoT2.Ard.Shared.
namespace {

riot2::NodeRuntime runtime;
ViewManager viewManager;
M5Canvas canvas(&M5Dial.Display);
DialInput input;
DisplayPower displayPower;
DiagnosticsScreen diagnostics;
RfidReader rfid;
bool showDiagnostics = false;
WifiState lastWifiState = WifiState::Disconnected;
MqttState lastMqttState = MqttState::Disconnected;

// On this PSRAM-less ESP32-S3 a full-screen 16bpp sprite plus Wi-Fi and the
// BLE controller can starve the heap until M5GFX's PNG decoder silently fails
// to draw icons (BUG fixed #3 in repo notes). So the canvas drops to 8bpp
// (RGB332, ~58KB vs ~115KB) once BLE runs, and keeps full color otherwise.
// createSprite() reallocates, so this can run again.
void configureCanvasColorDepth() {
    if (runtime.bleActive()) {
        canvas.setColorDepth(8);
    }
    canvas.createSprite(M5Dial.Display.width(), M5Dial.Display.height());
}

riot2::NodeRuntimeHooks runtimeHooks() {
    riot2::NodeRuntimeHooks hooks;
    hooks.viewTemplates = [] { return ViewFactory::instance().configurationTemplates(); };
    hooks.isView = [](const String& classFullName) { return ViewFactory::instance().isRegistered(classFullName); };
    hooks.onConfiguration = [](const NodeConfiguration& configuration) {
        viewManager.rebuild(configuration);
        rfid.setActive(viewManager.hasRfidConsumer());
        return viewManager.hasBleConsumer();
    };
    hooks.onCommand = [](const Command& command) {
        // An alert view (AlertView/NotificationView) took over the display: make
        // sure it is seen, not left on a dimmed or sleeping screen.
        if (viewManager.onCommand(command.id, command)) {
            displayPower.wake();
        }
    };
    hooks.onOtaStarted = [](const String&) {
        Buzzer::confirm();
        showStatus("Updating...", "Do not power off");
    };
    hooks.onOtaFailed = [] {
        Buzzer::error();
        showStatus("Update failed", "See serial log");
    };
    hooks.onBleStarted = configureCanvasColorDepth;
    hooks.onBleDeviceDiscovered = [](const BleDeviceInfo& device) { viewManager.notifyBleDeviceDiscovered(device); };
    hooks.onBleDeviceLost = [](const String& address) { viewManager.notifyBleDeviceLost(address); };
    hooks.onBleAdvertisement = [](const BleAdvertisement& advertisement) { viewManager.notifyBleAdvertisement(advertisement); };
    return hooks;
}

void factoryReset() {
    Buzzer::error();
    showStatus("Resetting...", "Clearing config");
    runtime.factoryReset();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    // First, so a new OTA image's trial time starts at boot (see riot2/OtaRollback.h).
    riot2::NodeRuntime::startOtaTrial();
    registerPeripherals();

    auto cfg = M5.config();
    // enableEncoder=false: DialInput reads the encoder pins with the hardware
    // PCNT decoder instead of M5Dial's software one.
    M5Dial.begin(cfg, false, false);
    Buzzer::begin();
    input.begin();

    runtime.begin(Manifest::json, runtimeHooks());
    if (runtime.isProvisioning()) {
        showStatus("Setup needed", "WiFi: " + runtime.provisioning().apSsid());
        return;
    }

    showStatus("Starting...", runtime.config().name);
    displayPower.begin();
    // BLE is never active this early, so the sprite starts at full color depth.
    configureCanvasColorDepth();
    viewManager.onReport([](const Report& report) { runtime.publishReport(report); });
}

void loop() {
    M5Dial.update();

    runtime.loop();
    if (runtime.isProvisioning()) {
        return;
    }

    viewManager.loop();

    String uid = rfid.poll();
    // RFIDView takes over the display, as an alert does: make sure it is seen.
    if (uid.length() > 0 && viewManager.notifyRfidTagRead(uid)) {
        displayPower.wake();
    }

    WifiConnection& wifi = runtime.wifi();
    MqttConnection& mqtt = runtime.mqtt();
    bool statusChanged = wifi.state() != lastWifiState || mqtt.state() != lastMqttState;
    lastWifiState = wifi.state();
    lastMqttState = mqtt.state();

    DialInput::Frame frame = input.read();
    if (frame.factoryReset) {
        factoryReset();
        return;
    }
    if (frame.toggleDiagnostics) {
        showDiagnostics = !showDiagnostics;
        Buzzer::confirm();
    }

    bool hadInput = frame.hasInput();
    if (hadInput && displayPower.wake()) {
        hadInput = false;  // the first input after sleep only wakes the screen
    }
    displayPower.loop();

    if (showDiagnostics) {
        diagnostics.loop(canvas, runtime, frame.touchDown, frame.touchX, frame.touchY);
        canvas.pushSprite(0, 0);
        return;
    }
    diagnostics.hide();

    if (viewManager.hasViews()) {
        if (hadInput) {
            // One onEncoderChange() per physical click, so a fast spin moves as
            // many steps as it clicked (views only look at the sign).
            for (int i = 0; i < abs(frame.encoderClicks); ++i) {
                viewManager.onEncoderChange(frame.encoderClicks > 0 ? 1 : -1);
            }
            if (frame.buttonTapped) {
                viewManager.onButtonPress();
            }
            if (frame.touched) {
                viewManager.onTouch(frame.touchX, frame.touchY);
            }
        }
        viewManager.render(canvas);
        canvas.pushSprite(0, 0);
    } else if (statusChanged) {
        showStatus(wifi.isConnected() ? "WiFi: connected" : "WiFi: connecting...",
                   mqtt.isConnected() ? "MQTT: connected" : "MQTT: connecting...");
    }
}
