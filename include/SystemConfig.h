#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * SystemConfig: Persistente Speicherung der Controller- und RocNet-Knotenparameter
 * im ESP32 NVS-Flash (Non-Volatile Storage via ESP32 Preferences API).
 * Parameterstruktur orientiert an RocNetNode-Eigenschaften (Node-ID, Name, Ports).
 * ==============================================================================
 */

#include <Arduino.h>
#include "Config.h"

struct SystemSettings {
    uint16_t nodeId;
    char nodeName[32];
    int sdaPin;
    int sclPin;
    int ws2811Pin;
    uint16_t ws2811Chips;
    uint8_t signalBrightness;
    bool pairedPorts;
    char wifiSsid[33];
    char wifiPassword[65];
    char mdnsName[32];
    uint8_t logLevel; // 1 = ERROR (Default), 2 = DEBUG, 3 = INFO
};

class SystemConfig {
public:
    static SystemConfig& getInstance();

    // Load settings from NVS (or fall back to compile-time defaults)
    void begin();

    // Access current active settings
    const SystemSettings& get() const { return _settings; }
    SystemSettings& getMutable() { return _settings; }

    // Update settings in memory
    void set(const SystemSettings& settings);

    // Save current settings to NVS Flash
    bool save();

    // Reset settings to compile-time defaults in NVS
    void resetToDefaults();

private:
    SystemConfig();
    SystemSettings _settings;
    void loadDefaults();
};
