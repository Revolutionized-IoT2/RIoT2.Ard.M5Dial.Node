#include "BoardPeripherals.h"

#include <memory>

#include <riot2/GpioPeripheral.h>
#include <riot2/PeripheralFactory.h>
#include <riot2/Uuid.h>

namespace {
// M5Dial's Grove PORT.A / PORT.B pin table (GpioPeripheral itself is
// board-agnostic - see riot2/GpioPeripheral.h).
constexpr GpioPinMap kM5DialGroveMap{13, 15, 2, 1};
}  // namespace

void registerPeripherals() {
    PeripheralFactory::instance().registerCreator(
        "RIoT2.Ard.M5Dial.Node.GpioPeripheral", [] { return std::make_unique<GpioPeripheral>(kM5DialGroveMap); },
        [] {
            DeviceConfiguration config;
            config.id = riot2::newId();
            config.name = "GPIO Peripheral";
            config.classFullName = "RIoT2.Ard.M5Dial.Node.GpioPeripheral";
            config.deviceParameters = {{"pullup", "true"}, {"invert", "false"}};

            ReportTemplate report;
            report.id = riot2::newId();
            report.type = riot2::ValueType::Boolean;
            report.name = "Doorbell Button";
            report.address = "A1";
            config.reportTemplates.push_back(report);

            CommandTemplate cmd;
            cmd.id = riot2::newId();
            cmd.type = riot2::ValueType::Boolean;
            cmd.name = "Porch Relay";
            cmd.address = "A2";
            config.commandTemplates.push_back(cmd);
            return config;
        });
}
