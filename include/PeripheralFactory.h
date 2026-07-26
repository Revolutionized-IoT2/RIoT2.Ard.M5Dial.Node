#pragma once

#include <Arduino.h>

#include <functional>
#include <memory>
#include <vector>

#include "IPeripheral.h"

// Registry mapping a peripheral configuration's classFullName to a factory
// function that creates the matching IPeripheral implementation - the
// non-visual counterpart to ViewFactory. Concrete peripheral drivers (e.g.
// GpioPeripheral) register themselves here via a static initializer in
// their own .cpp file.
class PeripheralFactory {
public:
    using Creator = std::function<std::unique_ptr<IPeripheral>()>;

    static PeripheralFactory& instance();

    void registerPeripheral(const String& classFullName, Creator creator);

    // True if a peripheral is registered for classFullName - lets other
    // systems (e.g. ViewManager) check ownership without constructing an
    // instance just to test for one.
    bool isRegistered(const String& classFullName) const;

    // Returns nullptr if no peripheral is registered for classFullName.
    std::unique_ptr<IPeripheral> create(const String& classFullName) const;

private:
    std::vector<std::pair<String, Creator>> _creators;
};
