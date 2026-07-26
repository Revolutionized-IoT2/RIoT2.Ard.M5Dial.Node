#include "GpioPeripheral.h"

#include <memory>

#include "PeripheralFactory.h"

namespace {

// Resolves a Grove-port address ("A1"/"A2"/"B1"/"B2") to the matching GPIO
// pin, per the M5Stack Dial PinMap docs' HY2.0-4P table (pin order exactly
// as listed there - PORT.B is documented as G2 then G1, not G1 then G2):
//   PORT.A: GND, 5V, G13, G15
//   PORT.B: GND, 5V, G2,  G1
// Returns -1 for any unrecognized address.
int8_t addressToPin(const String& address) {
    if (address == "A1") return 13;
    if (address == "A2") return 15;
    if (address == "B1") return 2;
    if (address == "B2") return 1;
    return -1;
}

}  // namespace

GpioPeripheral::Slot* GpioPeripheral::findOrCreateSlot(const String& address) {
    for (auto& slot : _slots) {
        if (slot.address == address) {
            return &slot;
        }
    }
    Slot slot;
    slot.address = address;
    _slots.push_back(slot);
    return &_slots.back();
}

void GpioPeripheral::begin(const DeviceConfiguration& config) {
    _slots.clear();
    _pullup = findParameter(config.deviceParameters, "pullup", "true") == "true";
    _invert = findParameter(config.deviceParameters, "invert", "false") == "true";

    // A commandTemplate's address makes its pin an OUTPUT; addresses seen
    // only on a reportTemplate stay an INPUT - see class comment.
    for (const auto& cmd : config.commandTemplates) {
        if (cmd.address.length() == 0) {
            continue;
        }
        Slot* slot = findOrCreateSlot(cmd.address);
        slot->command = cmd;
        slot->hasCommand = true;
        slot->isOutput = true;
    }

    for (const auto& report : config.reportTemplates) {
        if (report.address.length() == 0) {
            continue;
        }
        Slot* slot = findOrCreateSlot(report.address);
        slot->report = report;
        slot->hasReport = true;
    }

    for (auto& slot : _slots) {
        slot.pin = addressToPin(slot.address);
        if (slot.pin < 0) {
            Serial.printf("[GpioPeripheral] Unrecognized Grove address \"%s\", ignoring\n", slot.address.c_str());
            continue;
        }

        if (slot.isOutput) {
            pinMode(slot.pin, OUTPUT);
            slot.lastState = false;
            digitalWrite(slot.pin, _invert ? HIGH : LOW);
        } else {
            pinMode(slot.pin, _pullup ? INPUT_PULLUP : INPUT);
            // Baseline the initial pin state without publishing a report -
            // only state changes *after* boot are reported.
            bool raw = digitalRead(slot.pin) == HIGH;
            slot.lastState = _invert ? !raw : raw;
            slot.pendingState = slot.lastState;
            slot.pendingSinceMs = millis();
        }
    }
}

void GpioPeripheral::loop() {
    unsigned long now = millis();
    for (auto& slot : _slots) {
        if (slot.pin < 0 || slot.isOutput || !slot.hasReport) {
            continue;
        }

        bool raw = digitalRead(slot.pin) == HIGH;
        bool value = _invert ? !raw : raw;

        if (value != slot.pendingState) {
            slot.pendingState = value;
            slot.pendingSinceMs = now;
        }

        if (value != slot.lastState && (now - slot.pendingSinceMs) >= kDebounceMs) {
            slot.lastState = value;
            publishReport(Report{slot.report.id, value ? "true" : "false"});
        }
    }
}

void GpioPeripheral::onCommand(const Command& command) {
    for (auto& slot : _slots) {
        if (slot.pin < 0 || !slot.isOutput || !slot.hasCommand || slot.command.id != command.id) {
            continue;
        }

        bool value = command.value.is<bool>() ? command.value.as<bool>() : command.value.as<int>() != 0;
        slot.lastState = value;
        digitalWrite(slot.pin, (_invert ? !value : value) ? HIGH : LOW);
        return;
    }
}

namespace {
struct GpioPeripheralRegistrar {
    GpioPeripheralRegistrar() {
        PeripheralFactory::instance().registerPeripheral("RIoT2.Ard.M5Dial.Node.GpioPeripheral",
                                                           []() { return std::make_unique<GpioPeripheral>(); });
    }
} gpioPeripheralRegistrar;
}  // namespace
