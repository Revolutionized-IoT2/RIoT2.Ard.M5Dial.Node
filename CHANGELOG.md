# Changelog

All notable documentation changes to `RIoT2.Ard.M5Dial.Node`.

## [Unreleased]

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
