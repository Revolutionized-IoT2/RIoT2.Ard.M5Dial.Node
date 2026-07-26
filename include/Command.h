#pragma once

#include <ArduinoJson.h>

#include "DeviceConfiguration.h"

// Mirrors RIoT2.Core.Models.Command: an inbound riot2/{id}/command message
// addressed to one of the active configuration's commandTemplates[].id.
// `value` is loosely-typed JSON (bool, number, string, or object) - inspect
// with JsonVariantConst::is<T>() / as<T>().
// `parameters` is populated by ViewManager::onCommand() from the matched
// commandTemplate's own `parameters` (command-specific config, e.g.
// AlertView/NotificationView's "soundEnabled"), distinct from the owning
// view's `deviceParameters`.
struct Command {
    String id;
    JsonVariantConst value;
    ParameterList parameters;
};
