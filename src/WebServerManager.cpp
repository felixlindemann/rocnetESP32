/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: WebServerManager Implementierung - Web-Dashboard und REST-Endpoints.
 * EN: WebServerManager implementation - web dashboard and REST API endpoints.
 * ==============================================================================
 */

#include "WebServerManager.h"
#include "WebUI.h"
#include "AppLog.h"
#include <WiFi.h>

WebServerManager::WebServerManager()
    : _server(80), _servoController(nullptr), _signalOutput(nullptr), _rocNetParser(nullptr) {
}

void WebServerManager::begin(ServoController* servoController, SignalOutput* signalOutput, RocNetParser* rocNetParser) {
    _servoController = servoController;
    _signalOutput = signalOutput;
    _rocNetParser = rocNetParser;

    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    _server.on("/api/set_modules", HTTP_POST, [this]() { handleApiSetModules(); });
    _server.on("/api/servo", HTTP_POST, [this]() { handleApiServo(); });
    _server.on("/api/test", HTTP_POST, [this]() { handleApiTest(); });
    _server.on("/api/save_all", HTTP_POST, [this]() { handleApiSaveAll(); });
    _server.on("/api/system", HTTP_GET, [this]() { handleApiGetSystem(); });
    _server.on("/api/system", HTTP_POST, [this]() { handleApiSaveSystem(); });
    _server.on("/api/reboot", HTTP_POST, [this]() { handleApiReboot(); });
    _server.on("/api/reset_system", HTTP_POST, [this]() { handleApiResetSystem(); });
    _server.on("/api/logs", HTTP_GET, [this]() { handleApiLogs(); });
    _server.on("/api/logs/level", HTTP_POST, [this]() { handleApiSetLogLevel(); });
    _server.on("/api/logs/level", HTTP_GET, [this]() { handleApiSetLogLevel(); });
    _server.on("/api/logs/clear", HTTP_POST, [this]() { handleApiClearLogs(); });
    _server.on("/api/identify", HTTP_POST, [this]() {
        if (_identifyCallback) _identifyCallback();
        _server.send(200, "application/json", "{\"status\":\"ok\"}");
    });
    _server.onNotFound([this]() { handleNotFound(); });

    _server.begin();
    LOG_D("HTTP WebServer started on port 80 (http://%s/)", WiFi.localIP().toString().c_str());
}

void WebServerManager::handleClient() {
    _server.handleClient();
}

void WebServerManager::handleRoot() {
    _server.send_P(200, "text/html", INDEX_HTML);
}

void WebServerManager::handleApiStatus() {
    if (!_servoController) {
        _server.send(500, "application/json", "{\"error\":\"Controller not ready\"}");
        return;
    }

    uint8_t modules = _servoController->getModuleCount();
    uint8_t total = _servoController->getTotalServos();
    uint8_t activeCount = _servoController->getActiveTurnoutCount();
    uint32_t totalSwitches = _servoController->getTotalSwitchCount();
    uint8_t highestPort = _servoController->getHighestTurnoutPort();
    uint16_t wsChips = _signalOutput ? _signalOutput->getChipCount() : 16;
    const auto& sys = SystemConfig::getInstance().get();
    bool paired = sys.pairedPorts;

    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "application/json", "");

    // Send header chunk
    String head = "{\"modules\":";
    head += modules;
    head += ",\"total\":";
    head += total;
    head += ",\"activeTurnouts\":";
    head += activeCount;
    head += ",\"highestTurnoutPort\":";
    head += highestPort;
    head += ",\"totalSwitches\":";
    head += totalSwitches;
    head += ",\"wsChips\":";
    head += wsChips;
    head += ",\"online\":[";
    for (uint8_t m = 0; m < MAX_PCA9685_MODULES; m++) {
        head += _servoController->isModuleOnline(m) ? "true" : "false";
        if (m < MAX_PCA9685_MODULES - 1) head += ",";
    }
    head += "],\"servos\":[";
    _server.sendContent(head);

    // Send servos in chunks of 4 to keep RAM usage minimal
    String chunk = "";
    for (uint8_t ch = 0; ch < total; ch++) {
        ServoChannelConfig cfg = _servoController->getChannelConfig(ch);
        ServoChannelState st = _servoController->getChannelState(ch);

        uint8_t board = (ch / SERVOS_PER_MODULE) + 1;
        uint8_t pin   = ch % SERVOS_PER_MODULE;
        uint16_t port1 = paired ? (ch * 2 + 1) : (ch + 1);
        uint16_t port2 = paired ? (ch * 2 + 2) : (ch + 1);

        char buf[220];
        snprintf(buf, sizeof(buf),
                 "{\"id\":%u,\"board\":%u,\"pin\":%u,\"port1\":%u,\"port2\":%u,"
                 "\"left\":%u,\"right\":%u,\"spd\":%u,\"inv\":%u,\"lastPos\":%u,\"switches\":%u}%s",
                 ch + 1, board, pin, port1, port2,
                 cfg.offPosAngle, cfg.onPosAngle, (unsigned int)cfg.travelDurationMs,
                 cfg.inverted ? 1 : 0, st.turnoutState ? 1 : 0, (unsigned int)st.switchCount,
                 (ch < total - 1) ? "," : "");

        chunk += buf;
        if ((ch % 4 == 3) || (ch == total - 1)) {
            _server.sendContent(chunk);
            chunk = "";
        }
    }

    _server.sendContent("]}");
}

void WebServerManager::handleApiSetModules() {
    if (!_servoController) {
        _server.send(500, "application/json", "{\"error\":\"Controller not ready\"}");
        return;
    }

    int count = _server.arg("count").toInt();
    if (count >= 1 && count <= MAX_PCA9685_MODULES) {
        _servoController->setModuleCount((uint8_t)count, true);
        log_i("WebUI set module count: %d (%d servos)", count, count * SERVOS_PER_MODULE);
        _server.send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        _server.send(400, "application/json", "{\"error\":\"Invalid module count\"}");
    }
}

void WebServerManager::handleApiServo() {
    if (!_servoController) {
        _server.send(500, "application/json", "{\"error\":\"Controller not ready\"}");
        return;
    }

    int id = _server.arg("id").toInt();
    if (id < 1 || id > _servoController->getTotalServos()) {
        _server.send(400, "application/json", "{\"error\":\"Invalid servo ID\"}");
        return;
    }

    uint8_t ch = id - 1;
    uint16_t left  = (uint16_t)_server.arg("left").toInt();
    uint16_t right = (uint16_t)_server.arg("right").toInt();
    uint32_t spd   = (uint32_t)_server.arg("spd").toInt();
    bool inv       = (_server.arg("inv").toInt() != 0);

    if (spd < 50) spd = 400;

    _servoController->programChannel(ch, left, right, 0, 0, inv ? 1 : 0, spd);

    log_i("WebUI saved Servo %d: Left=%u, Right=%u, Inverted=%d, Speed=%u ms",
          id, left, right, inv, spd);

    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleApiTest() {
    if (!_servoController) {
        _server.send(500, "application/json", "{\"error\":\"Controller not ready\"}");
        return;
    }

    int id = _server.arg("id").toInt();
    if (id < 1 || id > _servoController->getTotalServos()) {
        _server.send(400, "application/json", "{\"error\":\"Invalid servo ID\"}");
        return;
    }

    uint8_t ch = id - 1;
    int pos = _server.arg("pos").toInt();

    if (pos == 0) {
        _servoController->testServoLeft(ch);
    } else if (pos == 1) {
        _servoController->testServoRight(ch);
    } else if (pos == 2) {
        uint16_t angle = (uint16_t)_server.arg("angle").toInt();
        _servoController->testServoAngle(ch, angle);
    }

    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleApiSaveAll() {
    if (!_servoController) {
        _server.send(500, "application/json", "{\"error\":\"Controller not ready\"}");
        return;
    }

    _servoController->saveAllPreferences();
    log_i("WebUI: Saved all servos to Flash NVS");
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleApiGetSystem() {
    const auto& sys = SystemConfig::getInstance().get();
    String json = "{";
    json += "\"nodeId\":" + String(sys.nodeId) + ",";
    json += "\"nodeName\":\"" + String(sys.nodeName) + "\",";
    json += "\"sdaPin\":" + String(sys.sdaPin) + ",";
    json += "\"sclPin\":" + String(sys.sclPin) + ",";
    json += "\"ws2811Pin\":" + String(sys.ws2811Pin) + ",";
    json += "\"ws2811Chips\":" + String(sys.ws2811Chips) + ",";
    json += "\"signalBrightness\":" + String(sys.signalBrightness) + ",";
    json += "\"pairedPorts\":" + String(sys.pairedPorts ? "true" : "false") + ",";
    json += "\"wifiSsid\":\"" + String(sys.wifiSsid) + "\",";
    json += "\"mdnsName\":\"" + String(sys.mdnsName) + "\",";
    json += "\"logLevel\":" + String((int)sys.logLevel) + ",";
    json += "\"logLevelName\":\"" + String(AppLogger::getInstance().getLevelName()) + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime\":" + String(millis() / 1000);
    json += "}";
    _server.send(200, "application/json", json);
}

void WebServerManager::handleApiSaveSystem() {
    auto& sys = SystemConfig::getInstance().getMutable();
    bool needReboot = false;

    if (_server.hasArg("nodeId")) {
        uint16_t id = (uint16_t)_server.arg("nodeId").toInt();
        if (id > 0) {
            sys.nodeId = id;
            if (_rocNetParser) _rocNetParser->setNodeId(id);
        }
    }
    if (_server.hasArg("nodeName")) {
        String name = _server.arg("nodeName");
        name.trim();
        if (name.length() > 0) {
            strncpy(sys.nodeName, name.c_str(), sizeof(sys.nodeName) - 1);
            sys.nodeName[sizeof(sys.nodeName) - 1] = '\0';
        }
    }
    if (_server.hasArg("logLevel")) {
        int lvl = _server.arg("logLevel").toInt();
        if (lvl >= 1 && lvl <= 3) {
            sys.logLevel = (uint8_t)lvl;
            AppLogger::getInstance().setLevel(static_cast<AppLogLevel>(lvl));
            LOG_D("WebUI: LogLevel set to %s (%d)", AppLogger::getInstance().getLevelName(), lvl);
        }
    }
    if (_server.hasArg("sdaPin")) {
        int pin = _server.arg("sdaPin").toInt();
        if (pin != sys.sdaPin) { sys.sdaPin = pin; needReboot = true; }
    }
    if (_server.hasArg("sclPin")) {
        int pin = _server.arg("sclPin").toInt();
        if (pin != sys.sclPin) { sys.sclPin = pin; needReboot = true; }
    }
    if (_server.hasArg("ws2811Pin")) {
        int pin = _server.arg("ws2811Pin").toInt();
        if (pin != sys.ws2811Pin) { sys.ws2811Pin = pin; needReboot = true; }
    }
    if (_server.hasArg("ws2811Chips")) {
        uint16_t chips = (uint16_t)_server.arg("ws2811Chips").toInt();
        if (chips > 0 && chips <= 128) {
            sys.ws2811Chips = chips;
            if (_signalOutput) _signalOutput->setChipCount(chips);
        }
    }
    if (_server.hasArg("signalBrightness")) {
        int b = _server.arg("signalBrightness").toInt();
        if (b >= 0 && b <= 255) {
            sys.signalBrightness = (uint8_t)b;
            if (_signalOutput) _signalOutput->setBrightness((uint8_t)b);
        }
    }
    if (_server.hasArg("pairedPorts")) {
        bool paired = (_server.arg("pairedPorts") == "1" || _server.arg("pairedPorts") == "true");
        sys.pairedPorts = paired;
        if (_servoController) _servoController->setPairedPorts(paired);
    }
    if (_server.hasArg("wifiSsid")) {
        String ssid = _server.arg("wifiSsid");
        ssid.trim();
        if (ssid.length() > 0 && ssid != sys.wifiSsid) {
            strncpy(sys.wifiSsid, ssid.c_str(), sizeof(sys.wifiSsid) - 1);
            sys.wifiSsid[sizeof(sys.wifiSsid) - 1] = '\0';
            needReboot = true;
        }
    }
    if (_server.hasArg("wifiPassword")) {
        String pass = _server.arg("wifiPassword");
        if (pass.length() > 0 && pass != "********") {
            strncpy(sys.wifiPassword, pass.c_str(), sizeof(sys.wifiPassword) - 1);
            sys.wifiPassword[sizeof(sys.wifiPassword) - 1] = '\0';
            needReboot = true;
        }
    }
    if (_server.hasArg("mdnsName")) {
        String mdns = _server.arg("mdnsName");
        mdns.trim();
        if (mdns.length() > 0) {
            strncpy(sys.mdnsName, mdns.c_str(), sizeof(sys.mdnsName) - 1);
            sys.mdnsName[sizeof(sys.mdnsName) - 1] = '\0';
        }
    }

    SystemConfig::getInstance().save();

    bool userRequestedReboot = (_server.arg("reboot") == "1" || _server.arg("reboot") == "true");
    if (userRequestedReboot || needReboot) {
        _server.send(200, "application/json", "{\"status\":\"ok\",\"reboot\":true}");
        delay(400);
        ESP.restart();
    } else {
        _server.send(200, "application/json", "{\"status\":\"ok\",\"reboot\":false}");
    }
}

void WebServerManager::handleApiLogs() {
    uint32_t since = 0;
    if (_server.hasArg("since")) {
        since = (uint32_t)_server.arg("since").toInt();
    }
    String json = AppLogger::getInstance().getLogsJson(since);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleApiSetLogLevel() {
    int lvl = 1;
    if (_server.hasArg("level")) {
        lvl = _server.arg("level").toInt();
    }
    if (lvl >= 1 && lvl <= 3) {
        auto& sys = SystemConfig::getInstance().getMutable();
        sys.logLevel = (uint8_t)lvl;
        SystemConfig::getInstance().save();
        AppLogger::getInstance().setLevel(static_cast<AppLogLevel>(lvl));
        LOG_D("WebUI: Active LogLevel changed to %s (%d)", AppLogger::getInstance().getLevelName(), lvl);
        _server.send(200, "application/json", "{\"status\":\"ok\",\"level\":" + String(lvl) + ",\"levelName\":\"" + AppLogger::getInstance().getLevelName() + "\"}");
    } else {
        _server.send(400, "application/json", "{\"error\":\"Invalid log level (1..3)\"}");
    }
}

void WebServerManager::handleApiClearLogs() {
    AppLogger::getInstance().clear();
    _server.send(200, "application/json", "{\"status\":\"ok\",\"cleared\":true}");
}

void WebServerManager::handleApiReboot() {
    _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Rebooting\"}");
    delay(400);
    ESP.restart();
}

void WebServerManager::handleApiResetSystem() {
    SystemConfig::getInstance().resetToDefaults();
    _server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Reset to defaults, rebooting\"}");
    delay(400);
    ESP.restart();
}

void WebServerManager::handleNotFound() {
    _server.send(404, "text/plain", "404 Not Found");
}
