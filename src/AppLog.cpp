/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * AppLog Implementierung: Ringspeicher, Serial-Ausgabe und JSON-Generierung
 * ==============================================================================
 */

#include "AppLog.h"
#include <cstdio>
#include <cstring>

AppLogger& AppLogger::getInstance() {
    static AppLogger instance;
    return instance;
}

AppLogger::AppLogger()
    : _currentLevel(AppLogLevel::LOG_ERROR), // DEFAULT: Fehler
      _head(0),
      _count(0),
      _nextId(1) {
    memset(_entries, 0, sizeof(_entries));
}

void AppLogger::setLevel(AppLogLevel level) {
    if (level < AppLogLevel::LOG_ERROR) level = AppLogLevel::LOG_ERROR;
    if (level > AppLogLevel::LOG_INFO)  level = AppLogLevel::LOG_INFO;
    _currentLevel = level;
}

const char* AppLogger::getLevelName() const {
    return levelToString(_currentLevel);
}

const char* AppLogger::levelToString(AppLogLevel level) {
    switch (level) {
        case AppLogLevel::LOG_ERROR: return "ERROR";
        case AppLogLevel::LOG_DEBUG: return "DEBUG";
        case AppLogLevel::LOG_INFO:  return "INFO";
        default:                     return "UNKNOWN";
    }
}

void AppLogger::log(AppLogLevel level, const char* format, ...) {
    // Only process if message level is within active filter
    // LOG_ERROR = 1, LOG_DEBUG = 2, LOG_INFO = 3
    if (static_cast<uint8_t>(level) > static_cast<uint8_t>(_currentLevel)) {
        return;
    }

    char buf[140];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    uint32_t nowMs = 0;
#ifdef ARDUINO
    nowMs = millis();
    // Print to Serial Console
    const char* tag = levelToString(level);
    Serial.printf("[%7u ms] [%-5s] %s\n", nowMs, tag, buf);
#else
    printf("[%-5s] %s\n", levelToString(level), buf);
#endif

    // Save to circular ring buffer
    LogEntry& entry = _entries[_head];
    entry.id = _nextId++;
    entry.timestampMs = nowMs;
    entry.level = level;
    strncpy(entry.message, buf, sizeof(entry.message) - 1);
    entry.message[sizeof(entry.message) - 1] = '\0';

    _head = (_head + 1) % MAX_LOG_ENTRIES;
    if (_count < MAX_LOG_ENTRIES) {
        _count++;
    }
}

String AppLogger::getLogsJson(uint32_t sinceId) const {
    String json;
#ifdef ARDUINO
    json.reserve(2048);
#endif
    json += "{\"level\":";
    json += String((int)_currentLevel);
    json += ",\"levelName\":\"";
    json += getLevelName();
    json += "\",\"total\":";
    json += String((unsigned int)_count);
    json += ",\"entries\":[";

    bool first = true;
    size_t startIdx = (_count == MAX_LOG_ENTRIES) ? _head : 0;

    for (size_t i = 0; i < _count; i++) {
        size_t idx = (startIdx + i) % MAX_LOG_ENTRIES;
        const LogEntry& e = _entries[idx];
        if (e.id > sinceId) {
            if (!first) {
                json += ",";
            }
            first = false;

            json += "{\"id\":";
            json += String(e.id);
            json += ",\"ts\":";
            json += String(e.timestampMs);
            json += ",\"level\":\"";
            json += levelToString(e.level);
            json += "\",\"msg\":\"";
            // Escape json characters in msg
            for (size_t c = 0; e.message[c] != '\0'; c++) {
                char ch = e.message[c];
                if (ch == '"' || ch == '\\') {
                    json += '\\';
                    json += ch;
                } else if (ch == '\n') {
                    json += "\\n";
                } else if (ch == '\r') {
                    // skip
                } else {
                    json += ch;
                }
            }
            json += "\"}";
        }
    }

    json += "]}";
    return json;
}

void AppLogger::clear() {
    _head = 0;
    _count = 0;
    memset(_entries, 0, sizeof(_entries));
}
