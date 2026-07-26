#pragma once

#include <Arduino.h>

// Central palette of accent colors assigned to each view type - single
// source of truth shared between the home carousel (ViewManager, for the
// icon/circle color) and each view's own render() (for highlight/accent
// colors), so a view's on-screen accents match the color shown for it in
// the carousel instead of ad hoc per-view constants.
//
// Each view type has a Primary (bold) and Secondary (pale) color, matching
// the design palette below. The icon PNGs in Assets/icons/ already bake in
// each view's Primary color as their circle background, so Primary here is
// used for text/dot highlights and view accents; Secondary is used for
// pale/background accents (e.g. slider tracks, highlighted states).
namespace ViewColors {

struct RGB {
    uint8_t r, g, b;
};

const RGB Button = {26, 35, 126};                  // #1a237e
const RGB ButtonSecondary = {197, 202, 233};        // #c5cae9

const RGB ColorScheme = {74, 20, 140};              // #4a148c
const RGB ColorSchemeSecondary = {225, 190, 231};   // #e1bee7

const RGB Value = {0, 77, 64};                      // #004d40
const RGB ValueSecondary = {178, 223, 219};         // #b2dfdb

const RGB Percentage = {0, 96, 100};                // #006064
const RGB PercentageSecondary = {178, 235, 242};    // #b2ebf2

const RGB Toggle = {1, 87, 155};                    // #01579b
const RGB ToggleSecondary = {179, 229, 252};        // #b3e5fc

const RGB Slider = {191, 54, 12};                   // #bf360c
const RGB SliderSecondary = {255, 204, 188};        // #ffccbc

const RGB Scene = {27, 94, 32};                     // #1b5e20
const RGB SceneSecondary = {200, 230, 201};         // #c8e6c9

const RGB Clock = {49, 27, 146};                    // #311b92
const RGB ClockSecondary = {209, 196, 233};         // #d1c4e9

const RGB Alert = {183, 28, 28};                    // #b71c1c
const RGB AlertSecondary = {255, 205, 210};         // #ffcdd2

const RGB Notification = {13, 71, 161};             // #0d47a1
const RGB NotificationSecondary = {187, 222, 251};  // #bbdefb

const RGB Timer = {136, 14, 79};                    // #880e4f
const RGB TimerSecondary = {248, 187, 208};         // #f8bbd0

const RGB RFID = {245, 127, 23};                    // #f57f17
const RGB RFIDSecondary = {255, 249, 196};          // #fff9c4

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
