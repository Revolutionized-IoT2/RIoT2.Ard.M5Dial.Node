#pragma once

#include <Arduino.h>

#include <vector>

#include "IPeripheral.h"

// Drives up to 4 raw digital GPIO pins broken out on the M5Dial's two Grove
// (HY2.0-4P) ports - see the M5Stack Dial PinMap docs:
//   PORT.A: GND, 5V, G13, G15
//   PORT.B: GND, 5V, G2,  G1
// Each pin is addressed as "A1"/"A2" (PORT.A) or "B1"/"B2" (PORT.B) via a
// commandTemplate/reportTemplate's `address` field - the same address-based
// slot-correlation convention ButtonView/ToggleView use (see
// DeviceConfiguration.h). A pin is configured as:
//   - OUTPUT, if any commandTemplate references its address: an inbound
//     Command sets the pin HIGH/LOW (bool value). If a reportTemplate with
//     the same address also exists, it's currently unused for echoing (the
//     orchestrator already knows the value it just sent) - reserved for a
//     matching address with no commandTemplate, see below.
//   - INPUT (INPUT_PULLUP by default), if only a reportTemplate references
//     its address: the pin is polled and debounced in loop(), publishing a
//     Report on every confirmed state change - e.g. a button, PIR sensor,
//     reed switch, or other simple Grove digital module.
// deviceParameters:
//   - "pullup" ("true"/"false", default "true") - whether input pins use
//     their internal pull-up resistor.
//   - "invert" ("true"/"false", default "false") - flips the reported/
//     applied boolean sense for every pin on this peripheral (e.g. for
//     active-low modules where a physical LOW should read/apply as
//     logical `true`).
class GpioPeripheral : public IPeripheral {
public:
    void begin(const DeviceConfiguration& config) override;
    void loop() override;
    void onCommand(const Command& command) override;

private:
    struct Slot {
        String address;
        int8_t pin = -1;
        bool isOutput = false;
        bool hasReport = false;
        ReportTemplate report;
        bool hasCommand = false;
        CommandTemplate command;
        bool lastState = false;
        bool pendingState = false;
        unsigned long pendingSinceMs = 0;
    };

    static constexpr unsigned long kDebounceMs = 30;

    std::vector<Slot> _slots;
    bool _pullup = true;
    bool _invert = false;

    Slot* findOrCreateSlot(const String& address);
};
