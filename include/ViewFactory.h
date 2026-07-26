#pragma once

#include <Arduino.h>

#include <functional>
#include <memory>
#include <vector>

#include "IView.h"

// Registry mapping a DeviceConfiguration's classFullName (or name) to a
// factory function that creates the matching IView implementation.
//
// classFullName is treated as a string key into this registry, not a literal
// C++ class-loading mechanism - concrete views (Phase 5) register themselves
// here, typically via a static initializer in their own .cpp file.
class ViewFactory {
public:
    using Creator = std::function<std::unique_ptr<IView>()>;

    static ViewFactory& instance();

    void registerView(const String& classFullName, Creator creator);

    // True if a view is registered for classFullName - lets other systems
    // (e.g. PeripheralManager) check ownership without constructing an
    // instance just to test for one.
    bool isRegistered(const String& classFullName) const;

    // Returns nullptr if no view is registered for classFullName.
    std::unique_ptr<IView> create(const String& classFullName) const;

private:
    std::vector<std::pair<String, Creator>> _creators;
};
