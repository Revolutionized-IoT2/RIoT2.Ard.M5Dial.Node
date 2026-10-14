# RIoT2.Ard.M5Dial.Node

PlatformIO/Arduino firmware for the [M5Stack M5Dial](https://docs.m5stack.com/en/core/M5Dial) as
a RIoT2 device node. It connects to Wi-Fi and MQTT, announces itself to the orchestrator, fetches
device configuration and renders a rotary/touch UI for views such as buttons, sliders, timers,
scenes, RFID and BLE.

Most connectivity and protocol code lives in
[RIoT2.Ard.Shared](https://github.com/Revolutionized-IoT2/RIoT2.Ard.Shared). This repository owns
the M5Dial-specific canvas UI, rotary encoder, physical button gestures, buzzer feedback, built-in
RFID reader activation and Grove pin map.

## Hardware and prerequisites

- M5Stack M5Dial and USB-C cable.
- PlatformIO, either the VS Code extension or CLI.
- The sibling [RIoT2.Ard.Shared](https://github.com/Revolutionized-IoT2/RIoT2.Ard.Shared)
  repository checked out next to this one.
- Windows USB-serial drivers if the device is not detected automatically.

PlatformIO restores libraries from [platformio.ini](platformio.ini): `M5Dial`, `M5Unified`,
`PubSubClient`, `ArduinoJson`, `NimBLE-Arduino` and `ESP32Encoder`.

## Build

Run from the workspace root (`C:\Src\RIoT2`) in PowerShell:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $pio run -d .\RIoT2.Ard.M5Dial.Node
```

The firmware image is produced under `.pio\build\m5stack-stamps3\firmware.bin`.

To upload only when a device is connected and you intend to flash it:

```powershell
& $pio run -d .\RIoT2.Ard.M5Dial.Node -t upload
& $pio device monitor -d .\RIoT2.Ard.M5Dial.Node
```

Host regressions are in the shared repository:

```powershell
Set-Location C:\Src\RIoT2\RIoT2.Ard.Shared
python .\tests\test_firmware_p1.py
python .\tests\test_firmware_p2.py
python .\tests\test_firmware_architecture.py
```

## First boot provisioning

The firmware ships without Wi-Fi or MQTT credentials. On first boot, or after factory reset, it
starts an open setup access point named `RIoT2-Setup-XXXX`. The M5Dial display shows the setup
state and AP name.

Connect to the setup AP and open `http://192.168.4.1/`. The portal stores:

- node id and name;
- Wi-Fi SSID and password;
- MQTT URL, username and password;
- MQTT TLS flag;
- vibration feedback flag, carried for shared settings compatibility but unused by this board.

Settings are stored in ESP32 NVS namespace `riot2node`; see the hub
[firmware settings contract](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/env-vars.md#firmware-m5core2-m5dial).

To factory-reset provisioning, hold the physical dial button for about five seconds.

## UI and operation

- Rotate the encoder to move through carousel entries, then press the physical button to enter a
  view.
- Press the physical button from a focused view to return to the carousel.
- Hold the physical button for about 1.5 seconds to toggle diagnostics.
- In diagnostics, hold the on-screen power button for about two seconds to publish offline and
  power off.
- The display dims after about 15 seconds of inactivity and sleeps after about 60 seconds. The
  first wake input is swallowed.
- Built-in MFRC522 RFID is initialized only when the live configuration contains an RFID-consuming
  view; repeated reads of the same UID are suppressed for three seconds.
- BLE scanning starts only when a BLE-consuming view exists, then remains active.
- Grove pins are A1/GPIO13, A2/GPIO15, B1/GPIO2 and B2/GPIO1.

## OTA

Firmware OTA is triggered by an MQTT command to `riot2/node/{id}/command`:

```json
{ "id": "system.ota", "value": "https://host/path/to/firmware.bin" }
```

`system.ota` is reserved for firmware and is handled before view/peripheral dispatch. HTTPS
downloads use the compiled `RIOT2_ROOT_CA_PEM` when present; otherwise the firmware logs a warning
and uses insecure TLS.

### Automatic rollback

Since 0.2.0, a new image is on trial until it has connected to Wi-Fi and MQTT and fetched its
configuration from the orchestrator. If that doesn't happen within 3 minutes, or the image crashes
or restarts first, the device starts the previous image again. No cable is needed.

- The serial log shows `[OTA] New image in app0 is on trial` (or `app1`), then either `marked valid` or
  `rolling back`.
- Keep the orchestrator reachable during an update; an image that can't fetch its configuration
  rolls back. A device that seems stuck can be power-cycled: an unconfirmed image rolls back too.
  Don't factory-reset during the trial, for the same reason.
- A second `system.ota` is refused while an image is on trial, because it would overwrite the
  previous image that a rollback returns to.
- The previous image comes back as it was. Rollback protects updates *to* 0.2.0 or later; an older
  image marks itself valid at boot.
- The time limit is the build flag `RIOT2_OTA_VERIFY_TIMEOUT_MS` (milliseconds, default 180000).

To test it on a board running 0.2.0 or later:

1. Build a test image that never confirms itself, with a 60-second limit, and serve it on the LAN
   (from the workspace root, with `$pio` as in [Build](#build)):

   ```powershell
   $env:PLATFORMIO_BUILD_FLAGS = '-DRIOT2_OTA_TEST_NEVER_HEALTHY -DRIOT2_OTA_VERIFY_TIMEOUT_MS=60000UL'
   & $pio run -d .\RIoT2.Ard.M5Dial.Node
   Remove-Item Env:\PLATFORMIO_BUILD_FLAGS
   python -m http.server 8000 --directory .\RIoT2.Ard.M5Dial.Node\.pio\build\m5stack-stamps3
   ```

2. Send `{ "id": "system.ota", "value": "http://<pc>:8000/firmware.bin" }` to the node's command
   topic, and watch `& $pio device monitor -d .\RIoT2.Ard.M5Dial.Node`.
3. The new image logs `on trial`, connects and fetches its configuration, logs
   `RIOT2_OTA_TEST_NEVER_HEALTHY`, and after 60 seconds `rolling back`. The device restarts into
   the previous image, which comes online again with its old manifest version.
4. Rebuild without the flags (`& $pio run -d .\RIoT2.Ard.M5Dial.Node`) before you upload firmware
   for normal use.

## Troubleshooting

- **Upload fails or the port isn't found:** find the COM port with `& $pio device list`. Make
  sure no other program (a serial monitor or another IDE) has the port open.
- **The device stays on "Setup needed":** it has no valid stored configuration. Complete the
  provisioning above.
- **Stuck on "WiFi: connecting..." or "MQTT: connecting...":** check the credentials entered
  during provisioning. If needed, factory-reset (hold the button for 5 s) and provision again.
- **Diagnostics:** hold the physical button for about 1.5 s to toggle the diagnostics screen
  (Wi-Fi and MQTT status, signal strength, free heap).
- **Can't get back to the home menu:** press the physical button. It always returns to the home
  carousel; views only respond to touch and the bezel.

## Contracts and links

- [MQTT topics and payloads](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/mqtt-topics.md)
- [Firmware configuration subset](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/configuration.md#firmware-subset)
- [Firmware HTTP behavior](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/http-api.md#firmware-riot2ardshared)
- [Architecture overview](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/architecture/overview.md)
- AI coding instructions: [AGENTS.md](AGENTS.md)
- Release notes: [CHANGELOG.md](CHANGELOG.md)

## License

See [LICENSE](LICENSE).
