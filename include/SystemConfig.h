#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: SystemConfig: Persistente Speicherung der Controller- und RocNet-Parameter
 *     im ESP32 NVS-Flash (Non-Volatile Storage via ESP32 Preferences API).
 *     Parameterstruktur orientiert an RocNetNode-Eigenschaften (Node-ID, Name, Ports).
 * EN: SystemConfig: Persistent storage of controller and RocNet parameters
 *     in ESP32 NVS Flash (Non-Volatile Storage via ESP32 Preferences API).
 *     Parameter structure based on RocNetNode properties (Node ID, Name, Ports).
 * ==============================================================================
 */

#include <Arduino.h>
#include "Config.h"

// DE: Systemeinstellungs-Struktur / EN: System configuration structure
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
    uint8_t logLevel; // DE: 1 = FEHLER (Default), 2 = DEBUG, 3 = INFO / EN: 1 = ERROR, 2 = DEBUG, 3 = INFO
};

class SystemConfig {
public:
    static SystemConfig& getInstance();

    // DE: Einstellungen aus NVS laden (oder Default-Werte) / EN: Load settings from NVS (or fall back to defaults)
    void begin();

    // DE: Zugriff auf aktive Einstellungen / EN: Access current active settings
    const SystemSettings& get() const { return _settings; }
    SystemSettings& getMutable() { return _settings; }

    // DE: Einstellungen im RAM aktualisieren / EN: Update settings in memory
    void set(const SystemSettings& settings);

    // DE: Aktuelle Einstellungen im NVS-Flash speichern / EN: Save current settings to NVS Flash
    bool save();

    // DE: Einstellungen im NVS auf Standard zurücksetzen / EN: Reset settings to compile-time defaults
    void resetToDefaults();

private:
    SystemConfig();
    SystemSettings _settings;
    void loadDefaults();
};
