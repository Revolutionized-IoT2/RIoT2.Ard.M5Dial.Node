# CLAUDE.md

This file provides guidance to AI coding assistants when working with code in this repository.

## Project Overview

`RIoT2.Ard.M5Dial.Node` is a **PlatformIO / Arduino (C++) firmware project** for the
[M5Stack M5Dial](https://docs.m5stack.com/en/core/M5Dial) — an ESP32-S3 device with a round
240x240 touch LCD, a rotary encoder, a physical push button, and a buzzer.

It is a **Node** in the [RIoT2](../RIoT2.Core) ecosystem: a small edge device that connects to
Wi-Fi and MQTT, announces itself to the RIoT2 Orchestrator, downloads its device configuration,
and renders a rotary-dial UI that lets a user view and control remote devices (lights, scenes,
sensors, etc.) via MQTT reports/commands.

This repository currently contains only the default PlatformIO scaffold ([src/main.cpp](src/main.cpp)).
Everything described in **Architecture** and **Roadmap** below is the target design to build
towards, not yet-implemented fact — treat it as the spec to follow.

## Tech Stack

- **Build system:** [PlatformIO](https://platformio.org/) (see [platformio.ini](platformio.ini))
- **Board / platform:** `m5stack-stamps3` (ESP32-S3), `framework = arduino`
- **Hardware libraries:** `m5stack/M5Dial`, `m5stack/M5Unified` (display, touch, encoder, button,
  buzzer, power management)
- **Not yet added, needed for the roadmap below:**
  - An MQTT client (e.g. `knolleary/PubSubClient` or `256dpi/MQTT`) — must support a Last Will
    and Testament (LWT) message.
  - A JSON library (e.g. `bblanchon/ArduinoJson`) for parsing configuration and MQTT payloads.
  - An HTTP client for fetching device configuration (`HTTPClient.h`, built into the ESP32
    Arduino core) plus `WiFiClientSecure` if the orchestrator uses HTTPS.
  - Persistent storage for provisioning (`Preferences.h` / NVS) to hold the config parameters
    below across reboots.

## Build, Run & Debug

```powershell
# Build firmware
pio run

# Build and flash to a connected M5Dial
pio run -t upload

# Open serial monitor (115200 baud, matches platformio.ini)
pio device monitor

# Build + upload + monitor in one step
pio run -t upload -t monitor

# Clean build artifacts
pio run -t clean
```

## Configuration Parameters

The node is configured with the following parameters (see **Roadmap** for how these should be
provisioned onto the device):

| Parameter | Description |
| --- | --- |
| `Id` | Unique node identifier (GUID), used as the MQTT client id and in all topic strings. |
| `WifiSsid` / `WifiPassword` | Wi-Fi network credentials. |
| `MqttServerUrl` | Address of the MQTT broker. |
| `MqttUsername` / `MqttPassword` | MQTT broker credentials. |

## Startup / Connectivity Lifecycle

1. **Wi-Fi** — On boot, connect to Wi-Fi using `WifiSsid` / `WifiPassword`. Block/retry with
   backoff until connected (show connection status on the dial screen).
2. **MQTT** — Once Wi-Fi is up, connect to `MqttServerUrl` with `MqttUsername` / `MqttPassword`,
   using `Id` as the client id. Configure the **LWT** to publish `{"isOnline": false}` to
   `riot2/node/{id}/online` (retained), so a dirty disconnect (crash, power loss, network drop) is
   reported automatically by the broker without needing more app code.
3. **Announce online** — On successful MQTT connect, publish (retained):
   - Topic: `riot2/node/{id}/online`
   - Payload: `{ "name": "<node name>", "isOnline": true, "nodeType": 1 }`
     (`nodeType: 1` = `Device`, matching `RIoT2.Core.Enums.NodeType`.)
4. **Subscribe** to `riot2/orchestrator/online`, `riot2/node/{id}/configuration`, and
   `riot2/node/{id}/command`.
5. **Orchestrator handshake** — The orchestrator announces itself with an **empty** message on
   `riot2/orchestrator/online`. On receiving it, the node simply **re-publishes its own online
   message** (step 3, retained) to `riot2/node/{id}/online` — it does not fetch configuration yet.
   Once the orchestrator sees that online message, it replies with
   `riot2/node/{id}/configuration` (`{"apiBaseUrl": "..."}`). On receiving *that* message, the
   node `GET {apiBaseUrl}api/Nodes/{id}/configuration` over HTTP(S), parses the JSON, and
   (re)builds the UI from `deviceConfigurations`.
6. **Graceful shutdown** — On any controlled termination path (e.g. reset request), explicitly
   publish `{"isOnline": false}` to `riot2/node/{id}/online` before closing the MQTT connection,
   rather than relying solely on the LWT.
7. **Reconnect** — On Wi-Fi or MQTT disconnect, retry with backoff; re-subscribe and re-send the
   online message after MQTT reconnects.

> **Ecosystem note:** Topic strings are confirmed against `RIoT2.Core.Constants`
> (`../RIoT2.Core/Constants.cs`), which uses a `node/` segment:
> `riot2/node/{id}/online`, `riot2/node/{id}/command`, `riot2/node/{id}/report`. These are
> centralized in [include/Topics.h](include/Topics.h) so any future mismatch is a one-line fix.

## MQTT Message Contracts

| Direction | Topic | Payload |
| --- | --- | --- |
| Publish (retained) | `riot2/node/{id}/online` | `{ "name": string, "isOnline": bool, "nodeType": 1 }` |
| Publish (retained), on disconnect/shutdown | `riot2/node/{id}/online` | `{ "isOnline": false }` |
| Subscribe | `riot2/orchestrator/online` | *(empty)* — triggers node to re-publish its online message |
| Subscribe | `riot2/node/{id}/configuration` | `{ "apiBaseUrl": string }` — triggers the node to fetch its configuration |
| Subscribe (inbound commands) | `riot2/node/{id}/command` | `{ "id": string, "value": any }` |
| Publish (outbound reports) | `riot2/node/{id}/report` | `{ "id": string, "timeStamp": long, "value": any }` |

`value` is a loosely-typed JSON value (bool, number, string, or object) — mirrors `ValueModel` in
`RIoT2.Core`. `timeStamp` is Unix epoch seconds. `id` in commands/reports refers to a
`commandTemplate.id` / `reportTemplate.id` from the device configuration below, **not** the node
`Id`.

> **Node-specific extension (not part of `RIoT2.Core`):** the inbound command id `"system.ota"`
> is reserved. `{ "id": "system.ota", "value": "<firmware .bin url>" }` on
> `riot2/node/{id}/command` triggers an OTA update (see [include/OtaUpdater.h](include/OtaUpdater.h))
> instead of being routed to a view. This id is intercepted in `main.cpp::handleCommand()` before
> `viewManager.onCommand()` is called, so no view/device should ever use `"system.ota"` as one of
> its own `commandTemplate.id`s.

## Device Configuration (from Orchestrator)

Fetched via `GET {apiBaseUrl}api/Nodes/{id}/configuration`:

```json
{
  "name": "M5Dial Node",
  "id": "98D067AB-9672-45D1-81DF-110B4B82455D",
  "deviceConfigurations": [
    {
      "id": "A748CA9F-CF88-4255-9026-461BB8F92A14",
      "name": "ButtonView",
      "classFullName": "RIoT2.Ard.M5Dial.Node.ButtonView",
      "commandTemplates": [
        { "id": "...", "type": "0", "name": "Olohuone 1", "address": "btn-1", "valueType": 0, "model": false }
      ],
      "reportTemplates": [
        { "id": "...", "type": "0", "name": "Olohuone", "address": "btn-1", "parameters": { "icon": "p" } }
      ],
      "deviceParameters": { "header": "Button view", "subHeader": "sub header", "menuSubHeader": "menu subtitle" }
    }
  ]
}
```

- Each entry in `deviceConfigurations` maps to one **View** (a full-screen UI shown when the user
  dials to it) and one on-device "device" instance.
- `classFullName` identifies which View implementation to instantiate — treat it as a string key
  into a local View factory/registry (`"ButtonView" -> ButtonView`, etc.), not a literal C++
  class-loading mechanism.
- `commandTemplates[].id` / `reportTemplates[].id` are the ids used in the `command`/`report`
  MQTT message `id` field for that specific control.
- `valueType` mirrors `RIoT2.Core.Enums.ValueType` (`0=Boolean, 1=Text, 2=Number, 3=Entity,
  4=TextArray`).
- `deviceParameters` is a free-form string dictionary for view-specific display config (labels,
  units, icons, etc.).

## UI / View Architecture

- **Home carousel:** The top-level screen is a vertically scrolling "belt"/coverflow list, styled
  after M5Dial-UserDemo's `app_more_menu` (`apps/app_more_menu`): every View gets one row (icon +
  name + view-type subtitle), stacked vertically and scrolled smoothly as the dial turns. The
  focused row sits at screen center at full size/brightness; rows further away shrink, dim, and
  bow rightward the further they are from center (`kArcAmplitude`/`kArcRange` in
  [include/ViewManager.h](include/ViewManager.h)), fading toward the round panel's edge instead
  of being hard-clipped — this scales to many Views far better than arranging every icon around a
  fixed ring or a big centered card (both get crowded/overlapping or waste space past a handful
  of entries), and there's no separate page-position indicator (e.g. a dot strip) any more since
  the list itself communicates position. A View is entered ("focused", filling the whole screen)
  two ways:
  - rotate the dial encoder to scroll to an entry, then touch the content to enter the
    current one; or
  - touch the content directly, which selects and enters it immediately (no separate confirm
    needed).

  Touching the top band moves to the previous entry; touching the bottom band moves to the next
  entry — the display is split into three horizontal touch bands (top = previous, bottom = next,
  middle = enter) rather than precise per-element hit circles, since that's easier to hit
  reliably on a small round touch panel. Turning the dial eases the whole list's scroll position
  toward the new active entry (`_scrollPosition` in [src/ViewManager.cpp](src/ViewManager.cpp)),
  taking the shortest wraparound path so looping past the first/last entry animates smoothly
  instead of sweeping across the whole list. Each View type has its own distinct color and real
  PNG icon (see "View Colors & Icons" below), keyed off `classFullName`; any unregistered
  `classFullName` falls back to a vivid color cycled by position plus a generic dot glyph —
  deliberately avoiding near-black/near-gray colors so no icon/title reads as invisible against
  the background. The subtitle uses the device configuration's `menuSubHeader` deviceParameter
  if present, otherwise the view's simple class name with the trailing "View" stripped (e.g.
  `SliderView` -> "Slider") — only the focused row shows its subtitle, to keep the scrolled-away
  rows compact.

  While a View is focused, the encoder is forwarded to it only if it claims the encoder via
  `isInteracting()` (e.g. an "adjust value" submode); otherwise encoder rotation does nothing
  (it no longer scrolls the list — that only happens on the carousel). Touch is forwarded
  directly to the focused View, which decides what it means — Views never need the physical
  button. The physical button is the "confirm" gesture instead: on the carousel, pressing it
  enters the currently highlighted entry (the same as touching the content); while a View is
  focused, pressing it instead always returns to the carousel, regardless of what the focused
  View does with touch/encoder input.
- **View interface:** Each View should implement a common interface, e.g.:
  - `begin(DeviceConfiguration&)` — one-time setup from config.
  - `onEnter()` / `onExit()` — lifecycle when the carousel focuses/unfocuses this view.
  - `onEncoderChange(delta)` / `onTouch(...)` — input handling (Views are driven only by touch
    and the bezel/encoder; the physical button is handled entirely by `ViewManager` — entering a
    view from the carousel or returning to it from a focused view — and is never routed to a
    View).
  - `onCommand(Command&)` — apply an inbound command addressed to one of this view's
    `commandTemplates`.
  - `render()` — draw to the M5Dial's canvas/sprite.
  - Views that produce telemetry expose a way for the app shell to publish a `Report`.
- **Rendering:** Use `M5Dial.Display` (M5GFX/LovyanGFX) with an off-screen sprite to avoid
  flicker; keep drawing cheap enough to run smoothly alongside MQTT/network processing in
  `loop()`.

### View Types (per user request)

1. **ButtonView** — 1–4 buttons. Press sends a `Report` (button id → pressed value). An inbound
   `Command` for a button's id sets its visual state (e.g. on/off/highlighted).
2. **ColorSchemeView** — user picks a color (e.g. wheel/swatches navigated with the dial); sends
   the selected color as a `Report`.
3. **ValueView** — displays 1–2 read-only values with unit labels; updated only via inbound
   `Command`; no reports.
4. **PercentageView** — tap/click to enter "adjust" mode, rotate the dial to change a 0–100%
   value (with visual arc feedback), tap again to confirm and return to the carousel; the value
   can also be pushed in from an inbound `Command`.

### Suggested Additional View Types

Based on common patterns in similar rotary/round-display smart-home UIs (M5Stack's own demos,
ESPHome/Home Assistant "smart knob" projects, Loxone Touch, Nest/Ecobee-style thermostat dials,
and media-remote widgets):

- **ToggleView / SwitchView** — a single large on/off switch (simpler single-device variant of
  `ButtonView`), good for one light/relay per view.
- **SliderView / RangeView** — like `PercentageView` but for an arbitrary numeric range + unit
  (not just 0–100%), e.g. brightness, volume, fan speed.
- **ClimateView** — temperature setpoint (dial-adjusted) plus current temperature and mode
  (heat/cool/auto), similar to a Nest/Ecobee dial.
- **ColorTemperatureView** — companion to `ColorSchemeView` for tunable-white lights (Kelvin
  slider via dial instead of hue/saturation).
- **SceneSelectorView / MenuView** — scroll a short text/icon list with the dial and confirm by
  tapping to trigger a scene or preset (sends a `Report` with the selected item's id).
- **MediaControlView** — play/pause/skip via tap, volume via dial rotation; now-playing text
  as an inbound `Command`.
- **TimerView** — set a countdown (dial to adjust minutes), start/cancel by tapping; reports
  remaining time or completion.
- **StatusOverviewView / DashboardView** — read-only glanceable summary of several
  `reportTemplates` (e.g. small icons/values), no interaction — useful as the "home" view.
- **AlertView / NotificationView** — transient full-screen *popups*, not menu items: they're
  excluded from the home carousel entirely (`ViewManager` filters out any view whose `isAlert()`
  returns `true` when building the navigable menu) and are only ever shown when their inbound
  `Command` arrives, interrupting whatever's currently on screen; auto-dismisses or requires a tap
  to acknowledge.
- **ClockView** — idle/screensaver view (time + maybe next event) shown after inactivity timeout
  or as `deviceConfigurations[0]`. Views doing ongoing background work while focused (e.g.
  `TimerView` actively counting down) can override `IView::keepsAwake()` to suppress this timeout,
  since a countdown's own completion (buzzer, report) only fires from inside its `render()` and
  would otherwise stall while the idle screen is shown instead.

## View Colors & Icons

Each known view type (keyed by `classFullName`) has an assigned **Primary** color, **Secondary**
color, and icon, forming a single design palette shared across the app:

- **Primary** — the view's main accent color. Used for carousel title text, dot-strip highlight,
  and each view's own "active"/value accent (`ViewColors::<Name>` in
  [include/ViewColors.h](include/ViewColors.h)).
- **Secondary** — a pale companion color, used for backgrounds/tracks/highlighted states
  (`ViewColors::<Name>Secondary`), e.g. `SliderView`'s track and `PercentageView`'s
  adjust-mode highlight.
- **Icon** — a 42×42 RGBA PNG in [Assets/icons/](Assets/icons/), embedded as a byte array in
  [src/Icons.cpp](src/Icons.cpp) / [include/Icons.h](include/Icons.h) and drawn via
  `M5Canvas::drawPng()` (see `drawViewIcon()` in [src/ViewManager.cpp](src/ViewManager.cpp)).
  Each icon PNG already bakes in that view's Primary color as a filled circle background with a
  white glyph on top, so the carousel draws it directly without a separate background fill.

| View | Primary | Secondary | Icon |
| --- | --- | --- | --- |
| `AlertView` | `#b71c1c` | `#ffcdd2` | `Assets/icons/Alert.png` |
| `ButtonView` | `#1a237e` | `#c5cae9` | `Assets/icons/Button.png` |
| `ClockView` | `#311b92` | `#d1c4e9` | `Assets/icons/Clock.png` |
| `ColorSchemeView` | `#4a148c` | `#e1bee7` | `Assets/icons/ColorScheme.png` |
| `NotificationView` | `#0d47a1` | `#bbdefb` | `Assets/icons/Notification.png` |
| `PercentageView` | `#006064` | `#b2ebf2` | `Assets/icons/Percentage.png` |
| `SceneSelectorView` | `#1b5e20` | `#c8e6c9` | `Assets/icons/SceneSelector.png` |
| `SliderView` | `#bf360c` | `#ffccbc` | `Assets/icons/Slider.png` |
| `TimerView` | `#880e4f` | `#f8bbd0` | `Assets/icons/Timer.png` |
| `ToggleView` | `#01579b` | `#b3e5fc` | `Assets/icons/Toggle.png` |
| `ValueView` | `#004d40` | `#b2dfdb` | `Assets/icons/Value.png` |

Any `classFullName` not in this table (an unrecognized/custom view) falls back to a vivid color
cycled by carousel position and a plain colored dot instead of a PNG icon — see the `kFallback`
palette in `styleForClassFullName()` in [src/ViewManager.cpp](src/ViewManager.cpp). When adding a
new built-in view type, add its Primary/Secondary colors to
[include/ViewColors.h](include/ViewColors.h), drop a matching 42×42 PNG in
[Assets/icons/](Assets/icons/), and wire both into `styleForClassFullName()`.

## Roadmap

### Phase 1 — Connectivity Foundation
- [x] Add MQTT (`PubSubClient`/`256dpi/MQTT`) and JSON (`ArduinoJson`) libraries to
      [platformio.ini](platformio.ini).
- [x] Define a `NodeConfig` struct for `Id`, `WifiSsid`, `WifiPassword`, `MqttServerUrl`,
      `MqttUsername`, `MqttPassword`, backed by `Preferences` (NVS).
- [x] Implement Wi-Fi connect with retry/backoff and on-screen status.
- [x] Implement MQTT connect (with LWT set to `{"isOnline": false}` on `riot2/{id}/online`),
      retry/backoff, and reconnect handling.
- [x] Publish the `NodeOnlineMessage` on connect; publish `{"isOnline": false}` on graceful
      shutdown paths.

### Phase 2 — Provisioning UX
- [x] Since the node has no way to receive `Id`/Wi-Fi/MQTT credentials out of the box, add a
      first-boot captive-portal / config-AP flow (e.g. `WiFiManager`-style) or a serial/BLE
      provisioning command so these five parameters can be set without reflashing.
- [x] Add a "factory reset" gesture (e.g. long-press boot button) to clear stored config.

### Phase 3 — Orchestrator Handshake
- [x] Subscribe to `riot2/orchestrator/online`; on message (empty payload), re-publish the
      node's own online message to `riot2/node/{id}/online`.
- [x] Subscribe to `riot2/node/{id}/configuration`; on message (`{"apiBaseUrl": "..."}`), `GET`
      the node's configuration via `apiBaseUrl + api/Nodes/{id}/configuration`.
- [x] Parse `deviceConfigurations` into an in-memory model (id, name, classFullName,
      commandTemplates, reportTemplates, deviceParameters).
- [x] Handle re-configuration pushes (rebuild the view list without a full reboot).

### Phase 4 — View Framework
- [x] Define the `IView` interface (`begin/onEnter/onExit/onEncoderChange/onTouch/
      onCommand/render`).
- [x] Implement the carousel/view-manager driven by the rotary encoder + touch (the physical
      button is reserved for returning to the carousel).
- [x] Implement a simple View factory keyed by `classFullName`/`name`.

### Phase 5 — Core Views
- [x] `ButtonView` (1–4 buttons, report on press, command sets state).
- [x] `ValueView` (1–2 read-only values + units, command-driven).
- [x] `PercentageView` (tap-to-adjust, dial to change, command-driven).
- [x] `ColorSchemeView` (color picker, report on selection).

### Phase 6 — MQTT Wiring & Resilience
- [x] Route inbound `riot2/node/{id}/command` messages to the active view's `onCommand` by matching
      `commandTemplates[].id`.
- [x] Route view-generated reports to `riot2/node/{id}/report` with correct `id`/`timeStamp`/`value`.
- [x] Confirm/align topic strings with `RIoT2.Core.Constants` (see note above).
- [x] Verify LWT + explicit offline publish both work (simulate power loss vs. clean reset).

### Phase 7 — Additional Views
- [x] Implement a first batch from **Suggested Additional View Types**
      (`ToggleView`, `SliderView`, `SceneSelectorView` are the most broadly useful next).
- [x] Add `ClockView` as an idle/home screen with an inactivity timeout.

### Phase 8 — Polish & Ops
- [x] OTA firmware update support.
- [x] Power management (dim/sleep display on inactivity, wake on touch/encoder/button).
- [x] On-device diagnostics screen (Wi-Fi/MQTT status, signal strength, free heap).
- [x] Buzzer feedback for confirm/error actions.

## Conventions

- Keep MQTT topic strings and JSON field names in one header (e.g. `Topics.h` / `Contracts.h`)
  rather than scattering literals, so they stay easy to reconcile with `RIoT2.Core.Constants`
  and the model classes in `RIoT2.Core.Models` (`NodeOnlineMessage`, `Command`, `Report`,
  `DeviceConfiguration`, `CommandTemplate`, `ReportTemplate`).
- Match `RIoT2.Core.Enums.ValueType` and `NodeType` numeric values exactly when encoding JSON
  (`valueType`/`nodeType` are plain integers on the wire, not strings).
- Prefer non-blocking patterns in `loop()` (state machines / millis()-based timers) over
  `delay()`, since Wi-Fi/MQTT servicing and UI rendering must interleave.
