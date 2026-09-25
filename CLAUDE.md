# RIoT2.Ard.M5Dial.Node

Guidance for coding agents working on the M5Stack M5Dial firmware.

## Current project shape

This is a PlatformIO/Arduino C++ firmware project for the M5Stack M5Dial (ESP32-S3, 240x240 round touch LCD, rotary encoder, push button, buzzer, built-in MFRC522 RFID reader and Grove ports). It is a RIoT2 node: it provisions Wi-Fi/MQTT credentials, connects to MQTT, announces itself to the RIoT2 Orchestrator, fetches device configuration, and renders a rotary UI driven by the encoder/touch/button.

Shared connectivity/protocol code lives in `..\RIoT2.Ard.Shared\RIoT2Shared` and is consumed with `lib_extra_dirs = ..\RIoT2.Ard.Shared`. Do not duplicate Wi-Fi, MQTT, provisioning, OTA, BLE scanner, GPIO peripheral, configuration retry, or topic-contract code in this repo unless there is a board-specific reason.

## Build and validation

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor
```

Native shared tests, when a C++14 host compiler is available:

```powershell
Set-Location ..\RIoT2.Ard.Shared
python tests\test_firmware_p1.py
python tests\test_firmware_p2.py
python tests\test_firmware_architecture.py
```

## Runtime lifecycle

1. Load `NodeConfig` from NVS (`Preferences`, namespace `riot2node`). Optional `DEFAULT_*` build flags can seed first-boot values.
2. If invalid, start the shared captive portal AP `RIoT2-Setup-XXXX`; the M5Dial screen shows the SSID. The portal stores node id/name, Wi-Fi credentials, MQTT broker credentials, MQTT TLS, and vibration preference (unused on M5Dial).
3. If valid, initialize display canvas, hardware PCNT rotary encoder (`ESP32Encoder`), View/Peripheral factories, Wi-Fi, config-template HTTP server, SNTP, MQTT callbacks, and MQTT.
4. MQTT publishes retained online state to `riot2/node/{id}/online`, subscribes to `riot2/orchestrator/online`, `riot2/node/{id}/configuration`, and `riot2/node/{id}/command`, and uses retained LWT/offline state on disconnect/shutdown.
5. Orchestrator handshake: `riot2/orchestrator/online` causes the node to re-publish online. The orchestrator then publishes `{ "apiBaseUrl": "..." }` to `riot2/node/{id}/configuration`; the main loop (not the MQTT callback) performs the blocking HTTP(S) fetch and rebuilds views/peripherals.
6. Configuration cache is intentionally disabled here (`OrchestratorClient{false}`), so the carousel stays empty until a live orchestrator fetch succeeds.

## MQTT contracts

Confirmed against `..\RIoT2.Core\Constants.cs` and models:

| Direction | Topic | Payload |
| --- | --- | --- |
| publish retained | `riot2/node/{id}/online` | `{ "name": string, "isOnline": true, "nodeType": 1, "nodeBaseUrl": "http://<ip>", "manifest": {...} }` |
| publish retained | `riot2/node/{id}/online` | `{ "isOnline": false }` for graceful shutdown/LWT |
| subscribe | `riot2/orchestrator/online` | Retained orchestrator online message; payload is ignored |
| subscribe | `riot2/node/{id}/configuration` | `{ "apiBaseUrl": string }` |
| subscribe | `riot2/node/{id}/command` | `{ "id": string, "value": any }` |
| publish | `riot2/node/{id}/report` | `{ "id": string, "timeStamp": long, "value": any }` |

`id` in commands/reports is a `commandTemplate.id` or `reportTemplate.id`, not the node id. The node-specific reserved command id `system.ota` triggers OTA before view/peripheral dispatch.

## Views and peripherals

Registered view class names include `ButtonView`, `ToggleView`, `SliderView`, `PercentageView`, `ColorSchemeView`, `ValueView`, `SceneSelectorView`, `ClockView`, `AlertView`, `NotificationView`, `TimerView`, `RFIDView`, and `BLEView` under the `RIoT2.Ard.M5Dial.Node.*` namespace. Unknown view class names are skipped unless a shared peripheral factory owns them.

The M5Dial Grove map is PORT.A `G13/G15` and PORT.B `G2/G1`. `RIoT2.Ard.M5Dial.Node.GpioPeripheral` can use addresses `A1`, `A2`, `B1`, `B2` as command outputs and/or debounced report inputs. Device parameters `pullup` and `invert` are strings (`"true"`/`"false"`).

Built-in RFID (`M5Dial.Rfid`) is only initialized when the live configuration contains an `RFIDView`; repeated reads of the same UID are suppressed for 3 seconds. BLE scanning is only started when a BLE-consuming view exists, and Wi-Fi modem sleep is kept enabled for Wi-Fi/BLE coexistence once BLE is active.

## UI/input notes

- The encoder uses ESP32 PCNT via `ESP32Encoder`, not M5Dial's software/PJRC decoder.
- The physical button enters the selected carousel view, returns from a focused view, toggles diagnostics on medium hold, and factory-resets on long hold.
- The diagnostics screen includes an on-screen hold-to-power-off button that publishes offline before `M5Dial.Power.powerOff()`.
- Timers run from `ViewManager::loop()` even while hidden, in diagnostics, or while a popup is active.

## Security and production notes

- MQTT TLS and HTTPS orchestrator/OTA validation use `RIOT2_ROOT_CA_PEM` when configured; without it the code logs and falls back to `setInsecure()`.
- The provisioning AP is currently open and serves HTTP; use only on trusted local setup networks or add AP authentication/encrypted provisioning before production rollout.
- OTA accepts a URL command and relies on transport security only; there is no firmware signature, version policy, or ESP32 rollback validation yet.
