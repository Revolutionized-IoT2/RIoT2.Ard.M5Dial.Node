#pragma once

#include <Arduino.h>

#include <utility>
#include <vector>

// In-memory model for the device configuration fetched from the RIoT2
// Orchestrator (GET {apiBaseUrl}api/Nodes/{id}/configuration). Mirrors
// RIoT2.Core.Models (CommandTemplate, ReportTemplate, DeviceConfiguration).

struct CommandTemplate {
    String id;
    String type;
    String name;
    String address;
    int valueType = 0;  // mirrors RIoT2.Core.Enums.ValueType
    bool model = false;
};

// Free-form string dictionary helper shared by ReportTemplate/DeviceConfiguration.
using ParameterList = std::vector<std::pair<String, String>>;

inline String findParameter(const ParameterList& params, const String& key, const String& defaultValue = "") {
    for (const auto& kv : params) {
        if (kv.first == key) {
            return kv.second;
        }
    }
    return defaultValue;
}

struct ReportTemplate {
    String id;
    String type;
    String name;
    String address;
    ParameterList parameters;
};

struct DeviceConfiguration {
    String id;
    String name;
    String classFullName;
    std::vector<CommandTemplate> commandTemplates;
    std::vector<ReportTemplate> reportTemplates;
    ParameterList deviceParameters;
};

struct NodeConfiguration {
    String name;
    String id;
    std::vector<DeviceConfiguration> deviceConfigurations;
};
