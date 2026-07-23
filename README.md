# RIoT2.Ard.M5Dial.Node

Firmware for the [M5Stack M5Dial](https://docs.m5stack.com/en/core/M5Dial) acting as a **Node**
in the [RIoT2](../RIoT2.Core) ecosystem. It connects to Wi-Fi and MQTT, announces itself to the
RIoT2 Orchestrator, downloads its device configuration, and renders a rotary-dial UI for viewing
and controlling remote devices (lights, scenes, sensors, etc.) via MQTT reports/commands.

See [CLAUDE.md](CLAUDE.md) for the full architecture, MQTT contracts, and roadmap.

## Prerequisites

- [PlatformIO](https://platformio.org/) — either the [VS Code extension](https://platformio.org/install/ide?install=vscode)
  or the standalone `pio` CLI.
- A USB-C cable and an [M5Stack M5Dial](https://docs.m5stack.com/en/core/M5Dial) device.
- (Windows) USB-serial drivers for the M5Dial's USB-to-UART chip if your OS doesn't detect the
  device automatically — see M5Stack's [driver download page](https://docs.m5stack.com/en/download).

No manual library installation is required — PlatformIO resolves all dependencies
(`m5stack/M5Dial`, `m5stack/M5Unified`, `PubSubClient`, `ArduinoJson`, plus core-bundled ESP32
libraries) from [platformio.ini](platformio.ini) on first build.

## Build

Using the PlatformIO CLI:

```powershell
# If `pio` isn't on your PATH (common on Windows), use the full path instead:
# & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run

pio run
```

Or in VS Code with the PlatformIO extension installed: open this folder, then use the
**PlatformIO: Build** command (checkmark icon in the status bar).

## Flash to the M5Dial

1. Connect the M5Dial to your computer via USB-C.
2. Build and upload in one step:

   ```powershell
   pio run -t upload
   ```

   If PlatformIO can't auto-detect the serial port, list available ports and pass one explicitly:

   ```powershell
   pio device list
   pio run -t upload --upload-port COM5
   ```
3. (Optional) Open the serial monitor to watch boot/connection logs (115200 baud):

   ```powershell
   pio device monitor
   ```
4. Or do build + upload + monitor in one step:

   ```powershell
   pio run -t upload -t monitor
   ```

## First boot: provisioning

The firmware ships with no Wi-Fi/MQTT credentials baked in. On first boot (or after a factory
reset), the M5Dial starts its own Wi-Fi access point and a captive-portal web form:

1. Power on the M5Dial. The screen shows **"Setup needed"** with an AP name like
   `RIoT2-Setup-XXXX`.
2. From a phone or laptop, connect to that Wi-Fi network.
3. Browse to the address shown on the device (or just open any HTTP page — the captive portal
   redirects you). Fill in:
   - **Id** — a unique node identifier (GUID) for this device.
   - **WifiSsid** / **WifiPassword** — your home/office Wi-Fi credentials.
   - **MqttServerUrl** — address of your MQTT broker.
   - **MqttUsername** / **MqttPassword** — MQTT broker credentials (if required).
4. Submit the form. The device saves the configuration to flash (NVS) and restarts into normal
   operation, connecting to your Wi-Fi and MQTT broker and then to the RIoT2 Orchestrator.

To re-enter provisioning later (e.g. to change networks), perform a **factory reset**: press and
hold the M5Dial's physical button (the clickable encoder/display, `BtnA`) for about 5 seconds.
This clears the stored configuration and restarts the device back into the setup flow.

## Updating firmware over the air (OTA)

Once a node is online, it doesn't need to be re-flashed over USB for future updates — an
operator/orchestrator can publish the following to the node's `riot2/node/{id}/command` topic:

```json
{ "id": "system.ota", "value": "http://host/path/to/firmware.bin" }
```

The node downloads and flashes the binary from that URL and reboots automatically on success. See
[CLAUDE.md](CLAUDE.md#mqtt-message-contracts) for details.

## Troubleshooting

- **Upload fails / port not found:** confirm the correct COM port with `pio device list`, and
  make sure no other program (serial monitor, another IDE) has the port open.
- **Device boots but stays on "Setup needed":** it has no valid stored configuration — complete
  the provisioning flow above.
- **Stuck on "WiFi: connecting..." / "MQTT: connecting...":** double-check the credentials
  entered during provisioning (factory reset and re-provision if needed).
- **On-device diagnostics:** press and hold the physical button for about 1.5 seconds (shorter
  than the factory-reset hold) to toggle a diagnostics screen showing Wi-Fi/MQTT status, signal
  strength, and free heap.
- **Stuck inside a view / can't get back to the home menu:** press the physical button - it
  always returns you to the home carousel, no matter what a view does with touch or the bezel.
  Views themselves only respond to touch and the bezel (rotary dial); the physical button is
  reserved exclusively for this "go back" gesture.
