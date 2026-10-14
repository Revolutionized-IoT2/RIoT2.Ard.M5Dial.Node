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
- CI: `.github/workflows/ci.yml` builds every firmware project and runs the Ard.Shared native
  tests on every push ([CI guide](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/guides/ci.md)).
- Release: bump `VERSION`, commit, then push the same `x.y.z` tag. `release.yml` attaches
  `firmware-m5stack-stamps3-<tag>.bin`, the manifest and `SHA256SUMS` to a GitHub release; serve the
  `.bin` to OTA from there.

## Layout

| Path | Contents |
|---|---|
| `platformio.ini` | `m5stack-stamps3` environment (platform `espressif32` pinned for OTA rollback), C++14 flags, shared-library path and board libraries |
| `MANIFEST_NAME`, `VERSION` | Inputs to the shared manifest generation script |
| `include/*View.h`, `src/*View.cpp` | M5Dial canvas views and templates; the shared view types render a `riot2::views` model (RIoT2.Ard.Shared), which also builds their template |
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
- Keep `OtaRollback::begin()` first in `setup()`, and `OtaRollback::markHealthy()` right after a
  successful `requestConfiguration()` in `loop()` (see RIoT2.Ard.Shared). Don't change the pinned
  `espressif32` platform without checking that its bootloader still has app rollback enabled.
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
- A new OTA image rolls back unless it fetches its configuration within 3 minutes
  ([README](README.md#automatic-rollback)). Restarting or factory-resetting the device during that
  time rolls it back as well.
- Template `type` is a number (`riot2::ValueType::Number`, not `"2"`); there is no `valueType`.

## Related work

- Backlog item [15](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/open-issues.md):
  duplicated firmware views and runtime wiring.
- Optional hardening [S9](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/optional-hardening.md):
  provisioning security, NVS encryption, signed OTA and fail-closed TLS.
- Accepted architecture item ([ADR 0011](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/adr/0011-firmware-noderuntime.md)) [A8](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/architecture/target.md#a8-share-a-firmware-noderuntime):
  shared firmware `NodeRuntime`.
- Plan [M5](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/plans/m05-firmware-node-runtime.md):
  shared firmware view logic and node runtime.
- [Feature ideas](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/features.md#firmware):
  Wiegand peripheral integration and OTA channel improvements.
