#include "PeripheralFactory.h"

PeripheralFactory& PeripheralFactory::instance() {
    static PeripheralFactory factory;
    return factory;
}

void PeripheralFactory::registerPeripheral(const String& classFullName, Creator creator) {
    _creators.push_back({classFullName, creator});
}

std::unique_ptr<IPeripheral> PeripheralFactory::create(const String& classFullName) const {
    for (const auto& entry : _creators) {
        if (entry.first == classFullName) {
            return entry.second();
        }
    }
    return nullptr;
}
