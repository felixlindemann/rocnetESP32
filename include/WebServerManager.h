#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * WebServerManager: Integrierter HTTP-Webserver zur Weichenjustierung,
 * Signaldiagnose und RocNet-Knotenkonfiguration via Browser.
 * Basiert auf ESP32 WebServer Library (LGPL / Arduino-ESP32).
 * ==============================================================================
 */

#include <Arduino.h>
#include <WebServer.h>
#include "ServoController.h"
#include "SignalOutput.h"
#include "RocNetParser.h"
#include "SystemConfig.h"

#include <functional>

class WebServerManager {
public:
    WebServerManager();

    // Start HTTP Web Server on port 80
    void begin(ServoController* servoController, SignalOutput* signalOutput = nullptr, RocNetParser* rocNetParser = nullptr);

    // Set callback for user-triggered Rocrail discovery/identify
    void setIdentifyCallback(std::function<void()> cb) { _identifyCallback = cb; }

    // Call continuously from loop()
    void handleClient();

private:
    WebServer _server;
    ServoController* _servoController;
    SignalOutput* _signalOutput;
    RocNetParser* _rocNetParser;
    std::function<void()> _identifyCallback;

    void handleRoot();
    void handleApiStatus();
    void handleApiSetModules();
    void handleApiServo();
    void handleApiTest();
    void handleApiSaveAll();
    void handleApiGetSystem();
    void handleApiSaveSystem();
    void handleApiReboot();
    void handleApiResetSystem();
    void handleApiLogs();
    void handleApiSetLogLevel();
    void handleApiClearLogs();
    void handleNotFound();
};
