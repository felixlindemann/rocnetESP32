/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * SystemConfig Implementierung: NVS-Persistierung der Controller-Konfiguration
 * ==============================================================================
 */

#include "SystemConfig.h"
#include <Preferences.h>

static const char* NVS_NAMESPACE = "rn_sys";

SystemConfig& SystemConfig::getInstance() {
    static SystemConfig instance;
    return instance;
}

SystemConfig::SystemConfig() {
    loadDefaults();
}

void SystemConfig::loadDefaults() {
    _settings.nodeId = DEFAULT_ROCNET_NODE_ID;
    strncpy(_settings.nodeName, DEFAULT_ROCNET_NODE_NAME, sizeof(_settings.nodeName) - 1);
    _settings.nodeName[sizeof(_settings.nodeName) - 1] = '\0';

    _settings.sdaPin = I2C_SDA_PIN;
    _settings.sclPin = I2C_SCL_PIN;
    _settings.ws2811Pin = WS2811_DATA_PIN;
    _settings.ws2811Chips = DEFAULT_WS2811_CHIP_COUNT;
    _settings.signalBrightness = DEFAULT_SIGNAL_BRIGHTNESS;
    _settings.pairedPorts = TURNOUT_PAIRED_PORTS;

    strncpy(_settings.wifiSsid, WIFI_SSID, sizeof(_settings.wifiSsid) - 1);
    _settings.wifiSsid[sizeof(_settings.wifiSsid) - 1] = '\0';

    strncpy(_settings.wifiPassword, WIFI_PASSWORD, sizeof(_settings.wifiPassword) - 1);
    _settings.wifiPassword[sizeof(_settings.wifiPassword) - 1] = '\0';

    strncpy(_settings.mdnsName, "rocnet-controller", sizeof(_settings.mdnsName) - 1);
    _settings.mdnsName[sizeof(_settings.mdnsName) - 1] = '\0';

    _settings.logLevel = 1; // 1 = ERROR (Default)
}

void SystemConfig::begin() {
    loadDefaults();

    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        log_w("SystemConfig: No NVS partition found or failed to open; using defaults");
        return;
    }

    if (prefs.isKey("node_id")) {
        _settings.nodeId = prefs.getUShort("node_id", DEFAULT_ROCNET_NODE_ID);
    }
    if (prefs.isKey("node_name")) {
        String name = prefs.getString("node_name", DEFAULT_ROCNET_NODE_NAME);
        strncpy(_settings.nodeName, name.c_str(), sizeof(_settings.nodeName) - 1);
        _settings.nodeName[sizeof(_settings.nodeName) - 1] = '\0';
    }
    if (prefs.isKey("sda_pin")) {
        _settings.sdaPin = prefs.getInt("sda_pin", I2C_SDA_PIN);
    }
    if (prefs.isKey("scl_pin")) {
        _settings.sclPin = prefs.getInt("scl_pin", I2C_SCL_PIN);
    }
    if (prefs.isKey("ws_pin")) {
        _settings.ws2811Pin = prefs.getInt("ws_pin", WS2811_DATA_PIN);
    }
    if (prefs.isKey("ws_chips")) {
        _settings.ws2811Chips = prefs.getUShort("ws_chips", DEFAULT_WS2811_CHIP_COUNT);
    }
    if (prefs.isKey("brightness")) {
        _settings.signalBrightness = prefs.getUChar("brightness", DEFAULT_SIGNAL_BRIGHTNESS);
    }
    if (prefs.isKey("paired")) {
        _settings.pairedPorts = prefs.getBool("paired", TURNOUT_PAIRED_PORTS);
    }
    if (prefs.isKey("wifi_ssid")) {
        String ssid = prefs.getString("wifi_ssid", WIFI_SSID);
        strncpy(_settings.wifiSsid, ssid.c_str(), sizeof(_settings.wifiSsid) - 1);
        _settings.wifiSsid[sizeof(_settings.wifiSsid) - 1] = '\0';
    }
    if (prefs.isKey("wifi_pass")) {
        String pass = prefs.getString("wifi_pass", WIFI_PASSWORD);
        strncpy(_settings.wifiPassword, pass.c_str(), sizeof(_settings.wifiPassword) - 1);
        _settings.wifiPassword[sizeof(_settings.wifiPassword) - 1] = '\0';
    }
    if (prefs.isKey("mdns")) {
        String mdns = prefs.getString("mdns", "rocnet-controller");
        strncpy(_settings.mdnsName, mdns.c_str(), sizeof(_settings.mdnsName) - 1);
        _settings.mdnsName[sizeof(_settings.mdnsName) - 1] = '\0';
    }
    if (prefs.isKey("log_lvl")) {
        _settings.logLevel = prefs.getUChar("log_lvl", 1);
    }

    prefs.end();

    log_i("SystemConfig loaded: NodeID=%u, Name='%s', SDA=%d, SCL=%d, WS2811_GPIO=%d, Chips=%u, Paired=%d, LogLevel=%u",
          _settings.nodeId, _settings.nodeName, _settings.sdaPin, _settings.sclPin,
          _settings.ws2811Pin, _settings.ws2811Chips, _settings.pairedPorts, _settings.logLevel);
}

void SystemConfig::set(const SystemSettings& settings) {
    _settings = settings;
}

bool SystemConfig::save() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        log_e("SystemConfig: Failed to open NVS for writing!");
        return false;
    }

    prefs.putUShort("node_id", _settings.nodeId);
    prefs.putString("node_name", _settings.nodeName);
    prefs.putInt("sda_pin", _settings.sdaPin);
    prefs.putInt("scl_pin", _settings.sclPin);
    prefs.putInt("ws_pin", _settings.ws2811Pin);
    prefs.putUShort("ws_chips", _settings.ws2811Chips);
    prefs.putUChar("brightness", _settings.signalBrightness);
    prefs.putBool("paired", _settings.pairedPorts);
    prefs.putString("wifi_ssid", _settings.wifiSsid);
    prefs.putString("wifi_pass", _settings.wifiPassword);
    prefs.putString("mdns", _settings.mdnsName);
    prefs.putUChar("log_lvl", _settings.logLevel);

    prefs.end();
    log_i("SystemConfig: All settings saved to NVS Flash");
    return true;
}

void SystemConfig::resetToDefaults() {
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.clear();
        prefs.end();
    }
    loadDefaults();
    log_i("SystemConfig: Reset to factory defaults");
}
