# Changelog

All notable documentation changes to `RIoT2.Ard.M5Dial.Node`.

## [Unreleased]

## [0.6.0] - 2026-11-02

Wiegand card readers ([I42](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-3-4.md#i42-wiegand-peripheral-s)). Flash over OTA as before; nothing changes until a configuration adds
the new peripheral.

- Added: the *Wiegand Reader* peripheral (`RIoT2.Ard.M5Dial.Node.WiegandI2CPeripheral`, shared
  `WiegandI2CPeripheral`): a RIoT2.Ard.WiegandI2C module on Grove PORT.A, every card code as a text
  report; parameters `address` (`0x26`), `irq` (`B1`/`B2`, optional) and `format` (`hex`,
  `decimal`, `facility`).
- Build: `renovate.json` extends the shared Renovate preset ([I33](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-2.md#i33-supply-chain-s)). Renovate opens grouped
  update pull requests once the app is installed (MA7).
- Release: each firmware image gets `firmware-<env>-<version>.spdx.json`, an SPDX SBOM of the
  PlatformIO platform and the resolved libraries.

## [0.5.0] - 2026-11-01

Command results and desired state ([I19](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-2.md#i19-firmware-results-and-desired-state-m)), from RIoT2.Ard.Shared's `NodeRuntime`.

- Added: the node advertises `command-result/1` and `desired-state/1`, answers commands that carry
  a `correlationId` on `riot2/node/{id}/command/result`, and publishes its configuration status,
  retained, on `riot2/node/{id}/status`. The orchestrator now tracks its commands and sync state.
- Changed: a configuration command for the configuration the node already runs no longer fetches
  it; commands and configuration messages arrive at QoS 1; the configuration is fetched from
  `api/nodes/…` (divergence D5).
- Changed: a correlated `system.ota` is answered `executed` before the restart, and a redelivery
  after the restart is not installed again.
- Changed: `ViewManager::onCommand` returns whether a view has the command id, and reports an
  alert taking over the display separately (`tookOverDisplay`), which still wakes the screen.

## [0.4.0] - 2026-10-14

Shared node runtime ([I18](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-2.md#i18-firmware-noderuntime-m)). No behaviour change intended: the node lifecycle (settings,
provisioning, Wi-Fi, MQTT, configuration fetch and retry, templates, SNTP, OTA, peripherals and the
BLE start-up) now comes from RIoT2.Ard.Shared's `riot2::NodeRuntime`, the same code on both boards.

- Changed: `main.cpp` keeps board setup, the runtime hooks and the main loop; input, display power,
  the diagnostics screen, the RFID reader and the peripheral registration moved to their own files.

## [0.3.0] - 2026-10-14

Shared view models ([I17](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-2.md#i17-firmware-view-models-m)). The twelve views both boards have keep their look and
default templates; their logic now lives in RIoT2.Ard.Shared's `riot2::views` models, tested on the
host, and the views only render them.

- Changed: Slider values stay on the `step` grid from `min`, as on the M5Core2: commands are
  snapped, and so is the dial. When `max - min` isn't a multiple of `step`, the highest value is
  the last step below `max`.
- Fixed: a Toggle switch or Button without a report id no longer publishes a report with an empty
  id (and the button doesn't flash).
- Fixed: a Notification with a negative `durationMs` closes after 4 seconds; it stayed open.
- Fixed: the Button flash survives the `millis()` wrap after 49 days.
- Changed: ColorScheme reads a `#RRGGBB` value one channel at a time; malformed values give a
  different color than before.

## [0.2.0] - 2026-10-14

OTA rollback and numeric template types ([I13](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/increments/phase-0-1.md#i13-firmware-ota-rollback-s)).

- Added: automatic rollback of a bad OTA image (backlog item 6). A new image is on trial until it
  has fetched its configuration from the orchestrator; after 3 minutes without that, or after a
  crash or restart, the device starts the previous image again. A `system.ota` command is refused
  during the trial. See [Automatic rollback](README.md#automatic-rollback), which also describes
  how to test it on a board.
- Fixed (divergence C2): the view and peripheral templates served to the orchestrator carry `type`
  as a number, and downloaded templates keep their numeric `type`. The unused `valueType` is gone.
- Changed: `platformio.ini` pins the `espressif32` platform (7.0.1, Arduino-ESP32 2.0.17), whose
  bootloader the rollback relies on.

## [0.1.0] - 2026-10-12

The first tagged release.

- Added CI and a release workflow: a `x.y.z` tag that matches `VERSION` attaches
  `firmware-m5stack-stamps3-<tag>.bin`, the manifest and `SHA256SUMS` to a GitHub release.
- Documentation: `AGENTS.md` is the AI instruction file, `CLAUDE.md` imports it,
  `.github/copilot-instructions.md` points to it, and no version notes were present in the README.

## Earlier versions

See git commits; there are no release notes or git tags for earlier versions.
