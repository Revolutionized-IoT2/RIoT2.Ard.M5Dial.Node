#pragma once

#include <Arduino.h>

#include <functional>

#include "DeviceConfiguration.h"

// Phase 3 orchestrator handshake.
//
// Given an apiBaseUrl (received on riot2/node/{id}/configuration, after the
// node has re-announced itself in response to riot2/orchestrator/online),
// fetches and parses {apiBaseUrl}api/Nodes/{id}/configuration into an
// in-memory NodeConfiguration, and notifies a callback so the rest of the
// app can rebuild itself from the new configuration without a full reboot.
//
// requestConfiguration() performs a blocking HTTP GET (bounded by a timeout).
// It only runs on the rare orchestrator (re)announce event, not on every
// loop() iteration, so blocking briefly is an acceptable trade-off for now;
// revisit if it turns out to starve MQTT/UI processing in practice.
class OrchestratorClient {
public:
    using ConfigurationCallback = std::function<void(const NodeConfiguration&)>;

    void onConfigurationUpdated(ConfigurationCallback callback) { _callback = callback; }

    const NodeConfiguration& current() const { return _current; }

    // Fetches and parses the configuration for nodeId from apiBaseUrl. Returns
    // false (and logs) on any HTTP/JSON error, leaving the previous
    // configuration untouched. On success, updates current() and invokes the
    // configuration callback.
    bool requestConfiguration(const String& apiBaseUrl, const String& nodeId);

private:
    static constexpr uint32_t kHttpTimeoutMs = 8000;

    ConfigurationCallback _callback;
    NodeConfiguration _current;

    bool parseConfiguration(const String& json, NodeConfiguration& out);
};
