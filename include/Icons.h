#pragma once

#include <Arduino.h>
#include <cstddef>

// Raw PNG byte arrays for each view's carousel/header icon, generated from
// the 42x42 PNGs in Assets/icons/. Definitions live in Icons.cpp; drawn via
// M5Canvas::drawPng() (see ViewManager.cpp's drawViewIcon()).
namespace Icons {

extern const uint8_t kAlertPng[];
extern const size_t kAlertPngLen;

extern const uint8_t kBLEPng[];
extern const size_t kBLEPngLen;

extern const uint8_t kButtonPng[];
extern const size_t kButtonPngLen;

extern const uint8_t kClockPng[];
extern const size_t kClockPngLen;

extern const uint8_t kColorSchemePng[];
extern const size_t kColorSchemePngLen;

extern const uint8_t kNotificationPng[];
extern const size_t kNotificationPngLen;

extern const uint8_t kPercentagePng[];
extern const size_t kPercentagePngLen;

extern const uint8_t kScenePng[];
extern const size_t kScenePngLen;

extern const uint8_t kSliderPng[];
extern const size_t kSliderPngLen;

extern const uint8_t kTimerPng[];
extern const size_t kTimerPngLen;

extern const uint8_t kTogglePng[];
extern const size_t kTogglePngLen;

extern const uint8_t kValuePng[];
extern const size_t kValuePngLen;

}  // namespace Icons
