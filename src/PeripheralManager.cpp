#include "PeripheralManager.h"

#include "PeripheralFactory.h"

void PeripheralManager::rebuild(const NodeConfiguration& nodeConfiguration) {
    _entries.clear();

    for (const auto& config : nodeConfiguration.peripheralConfigurations) {
        std::unique_ptr<IPeripheral> peripheral = PeripheralFactory::instance().create(config.classFullName);
        if (!peripheral) {
            Serial.printf("[PeripheralManager] No peripheral registered for classFullName=%s (id=%s)\n",
                          config.classFullName.c_str(), config.id.c_str());
            continue;
        }

        peripheral->setReportCallback(_reportCallback);
        peripheral->begin(config);

        Entry entry;
        entry.config = config;
        entry.peripheral = std::move(peripheral);
        _entries.push_back(std::move(entry));
    }

    Serial.printf("[PeripheralManager] Rebuilt %u peripheral(s)\n", static_cast<unsigned>(_entries.size()));
}

void PeripheralManager::loop() {
    for (auto& entry : _entries) {
        entry.peripheral->loop();
    }
}

bool PeripheralManager::onCommand(const String& commandId, const Command& command) {
    for (auto& entry : _entries) {
        for (const auto& cmdTemplate : entry.config.commandTemplates) {
            if (cmdTemplate.id == commandId) {
                entry.peripheral->onCommand(command);
                return true;
            }
        }
    }
    return false;
}
