#pragma once

#include <M5Dial.h>

// Simple audible feedback via the M5Dial's onboard speaker for confirm/error
// actions across views and the app shell. Non-blocking (M5Unified's Speaker
// mixes tones on a background task), safe to call from render/input handlers.
namespace Buzzer {

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

// A short bright "bell" tone. Callers that want a longer attention-grabbing
// pattern (e.g. TimerView's optional "egg timer ring" on completion) call
// this repeatedly from their own non-blocking timer/state machine rather
// than this function blocking itself.
inline void ring() {
    M5Dial.Speaker.tone(2600, 120);
}

}  // namespace Buzzer
