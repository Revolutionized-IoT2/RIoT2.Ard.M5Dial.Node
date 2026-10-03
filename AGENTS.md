# AGENTS.md — RIoT2.Ard.M5Dial.Node

Applies to: this repository. Read the platform guide first:
[.github/AGENTS.md](https://github.com/Revolutionized-IoT2/.github/blob/main/AGENTS.md). It covers the
workspace map, platform-wide rules and the documentation rules. In the local workspace, every
`https://github.com/Revolutionized-IoT2/<Repo>/blob/main/<path>` link is the file
`C:\Src\RIoT2\<Repo>\<path>`; read the local file instead of fetching the URL.

## What this is

PlatformIO/Arduino firmware for the M5Stack M5Dial RIoT2 node. It uses
`RIoT2.Ard.Shared` for Wi-Fi, MQTT, provisioning, configuration fetch, OTA, BLE and peripherals,
and adds the M5Dial canvas UI, hardware PCNT encoder, physical-button gestures, buzzer feedback,
built-in RFID reader activation and board pin map.

## Commands

Run from the workspace root (`C:\Src\RIoT2`), in PowerShell:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $pio run -d .\RIoT2.Ard.M5Dial.Node
```

Run shared native regressions from `C:\Src\RIoT2\RIoT2.Ard.Shared` when a C++14 compiler is
available:

```powershell
python .\tests\test_firmware_p1.py
python .\tests\test_firmware_p2.py
python .\tests\test_firmware_architecture.py
```

Device commands, only when an M5Dial is connected and flashing/monitoring was requested:

```powershell
& $pio run -d .\RIoT2.Ard.M5Dial.Node -t upload
& $pio device monitor -d .\RIoT2.Ard.M5Dial.Node
```

- Build artifact: `.pio\build\m5stack-stamps3\firmware.bin`.
- Run: there is no host runtime. Do not run upload/monitor unless the task explicitly involves a
  connected device.
- Release: no CI workflow or git tags are present. Update `VERSION`, build the `.bin`, and serve it
  to OTA if doing a manual release.

## Layout

| Path | Contents |
|---|---|
| `platformio.ini` | `m5stack-stamps3` environment, C++14 flags, shared-library path and board libraries |
| `MANIFEST_NAME`, `VERSION` | Inputs to the shared manifest generation script |
| `include/*View.h`, `src/*View.cpp` | M5Dial canvas views and templates |
| `include/ViewManager.h`, `src/ViewManager.cpp` | Carousel, focus, popups, hidden timers, RFID/BLE event routing |
| `include/Buzzer.h`, `src/main.cpp` | Buzzer feedback and board lifecycle wiring |
| `include/RFIDView.h`, `src/RFIDView.cpp` | Built-in RFID view |
| `src/Icons.cpp`, `include/Icons.h` | Embedded icon assets |
| `.github/copilot-instructions.md` | Pointer to this file only |

## Contracts consumed here

- [mqtt-topics.md](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/mqtt-topics.md):
  consumed through `RIoT2.Ard.Shared/RIoT2Shared/include/riot2/Topics.h` and `MqttConnection`.
- [configuration.md](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/configuration.md):
  view and peripheral templates in `src/*View.cpp`, `RFIDView.cpp` and the shared parser.
- [http-api.md](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/http-api.md):
  shared configuration fetch, provisioning portal and template server.
- [env-vars.md firmware section](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/env-vars.md#firmware-m5core2-m5dial):
  provisioning values stored in NVS by shared code.

## Rules

- Reusable logic must go in `RIoT2.Ard.Shared`, not only in this repository. If shared code changes,
  build both M5Dial and M5Core2.
- Keep encoder input on `ESP32Encoder`/PCNT, not a software decoder.
- Do not assign `system.ota` to a view command template.
- Do not turn RFID or BLE into unconditional hardware work; RFID is activated only by RFID views,
  and BLE starts only after a BLE-consuming view exists.
- Do not edit vendored `.pio\libdeps` content.
- Do not commit real Wi-Fi, MQTT, node id, broker or certificate values. Use placeholders in docs.
- Do not flash a device unless the user asks for upload/device validation.

## Pitfalls

- `OrchestratorClient` is constructed with `enableCache=false`; the carousel stays empty until a
  live orchestrator fetch succeeds and does not load cached configuration.
- The physical button has three timing-sensitive roles: tap enters/leaves views, medium hold
  toggles diagnostics, and long hold factory-resets provisioning.
- Hidden timers continue running from `ViewManager::loop()` even while another view, popup,
  diagnostics or idle state is displayed.
- RFID repeat suppression is three seconds for the same UID; BLE has no corresponding stop path
  after it starts.
- Firmware template `type` values downloaded from the orchestrator are lost because shared parsing
  reads `type` as a string while the contract sends a number (configuration divergence C2).

## Related work

- Backlog item [6](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/open-issues.md):
  OTA rollback policy.
- Backlog item [15](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/open-issues.md):
  duplicated firmware views and runtime wiring.
- Optional hardening [S9](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/optional-hardening.md):
  provisioning security, NVS encryption, signed OTA and fail-closed TLS.
- Architecture proposal [A8](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/architecture/target.md#a8-share-a-firmware-noderuntime):
  shared firmware `NodeRuntime`.
- Plan [M5](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/plans/m05-firmware-node-runtime.md):
  shared firmware view logic and node runtime.
- [Feature ideas](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/features.md#firmware):
  Wiegand peripheral integration and OTA channel improvements.
