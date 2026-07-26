#pragma once

#include <M5Dial.h>

// Simple audible feedback via the M5Dial's onboard speaker for confirm/error
// actions across views and the app shell. Non-blocking (M5Unified's Speaker
// mixes tones on a background task), safe to call from render/input handlers.
namespace Buzzer {

// M5Unified's default speaker master volume is only 64/255 - quite quiet on
// the M5Dial's small piezo speaker. Call once from setup() to turn it up to
// max; all tone()s below share this master volume.
inline void begin() {
    M5Dial.Speaker.setVolume(255);
}

// A light click, for momentary/toggle actions (ButtonView, ToggleView).
inline void tap() {
    M5Dial.Speaker.tone(1200, 40);
}

// A short rising tone, for a confirmed value/selection (Percentage/Slider/
// ColorScheme/SceneSelectorView "confirm" press, diagnostics toggle, OTA start).
inline void confirm() {
    M5Dial.Speaker.tone(1800, 80);
}

// A low buzz, for destructive/failure actions (factory reset, OTA failure).
inline void error() {
    M5Dial.Speaker.tone(300, 250);
}

// A brief two-tone "alarm" chord for AlertView's incoming-alert sound -
// brighter and noticeably louder than error() on the M5Dial's small piezo
// speaker: tiny piezo elements resonate far better in the ~2-3kHz range than
// in the bass range error() uses, and layering two tones on separate
// virtual channels (M5Unified's Speaker mixes them concurrently) carries
// more perceived loudness than a single tone at the same master volume.
inline void alert() {
    M5Dial.Speaker.tone(2200, 200, 0, true);
    M5Dial.Speaker.tone(2800, 200, 1, true);
}

// A short bright "bell" tone. Callers that want a longer attention-grabbing
// pattern (e.g. TimerView's optional "egg timer ring" on completion) call
// this repeatedly from their own non-blocking timer/state machine rather
// than this function blocking itself.
inline void ring() {
    M5Dial.Speaker.tone(2600, 120);
}

}  // namespace Buzzer
