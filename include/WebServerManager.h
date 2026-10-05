#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: WebServerManager: Integrierter HTTP-Webserver zur Weichenjustierung,
 *     Signaldiagnose und RocNet-Knotenkonfiguration via Browser.
 *     Basiert auf ESP32 WebServer Library (LGPL / Arduino-ESP32).
 * EN: WebServerManager: Embedded HTTP web server for servo finetuning,
 *     signal diagnostics, and RocNet node configuration via browser.
 *     Based on ESP32 WebServer Library (LGPL / Arduino-ESP32).
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

    // DE: HTTP-Webserver auf Port 80 starten / EN: Start HTTP web server on port 80
    void begin(ServoController* servoController, SignalOutput* signalOutput = nullptr, RocNetParser* rocNetParser = nullptr);

    // DE: Callback für Rocrail-Identify / EN: Set callback for user-triggered Rocrail discovery/identify
    void setIdentifyCallback(std::function<void()> cb) { _identifyCallback = cb; }

    // DE: Kontinuierlich im loop() aufrufen / EN: Call continuously from loop()
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
