#pragma once

#include <Arduino.h>

// Central palette of accent colors assigned to each view type - single
// source of truth shared between the home carousel (ViewManager, for the
// icon/circle color) and each view's own render() (for highlight/accent
// colors), so a view's on-screen accents match the color shown for it in
// the carousel instead of ad hoc per-view constants.
namespace ViewColors {

struct RGB {
    uint8_t r, g, b;
};

const RGB Button = {40, 110, 240};       // blue
const RGB ColorScheme = {190, 60, 220};  // purple
const RGB Value = {40, 180, 90};         // green
const RGB Percentage = {235, 140, 20};   // orange
const RGB Toggle = {20, 175, 175};       // teal
const RGB Slider = {225, 195, 25};       // yellow
const RGB Scene = {220, 60, 95};         // red/pink
const RGB Clock = {130, 130, 220};       // lavender

// Packs an RGB triple into the 16-bit 5-6-5 color M5GFX/LovyanGFX canvas
// drawing calls expect.
inline uint16_t toRGB565(const RGB& c) {
    return static_cast<uint16_t>(((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3));
}

// Returns `c` blended toward white by `amount` (0 = unchanged, 1 = white) -
// a brighter on-theme highlight for states like "currently being adjusted".
inline RGB lighten(const RGB& c, float amount) {
    auto mix = [amount](uint8_t v) { return static_cast<uint8_t>(v + (255 - v) * amount); };
    return RGB{mix(c.r), mix(c.g), mix(c.b)};
}

}  // namespace ViewColors
