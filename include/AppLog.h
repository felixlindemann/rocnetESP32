#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS & LIZENZ / SOURCE ATTRIBUTION & LICENSE:
 * ==============================================================================
 * DE: AppLog: 3-stufiges Logging- und Ringspeichersystem für RocNet ESP32.
 * EN: AppLog: 3-tier logging and circular ring buffer system for RocNet ESP32.
 *
 * Stufen / Levels:
 *   1. LOG_ERROR = DE: Nur kritische Fehler (DEFAULT) / EN: Critical errors only (DEFAULT)
 *   2. LOG_DEBUG = DE: Wesentliche Statusinfos (Weichen, Signale) / EN: Essential events
 *   3. LOG_INFO  = DE: Alle Details (Hexdumps, Heartbeats) / EN: Verbose details (hexdumps)
 *
 * DE: Enthält einen Ringspeicher für WebUI Live-Logging und REST-Endpoints.
 * EN: Contains an in-memory ring buffer for WebUI live streaming and REST endpoints.
 * ==============================================================================
 */

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#ifdef ARDUINO
#include <Arduino.h>
#else
#include <string>
class String : public std::string {
public:
    String() : std::string() {}
    String(const char* s) : std::string(s ? s : "") {}
    String(const std::string& s) : std::string(s) {}
    String(int val) : std::string(std::to_string(val)) {}
    String(unsigned int val) : std::string(std::to_string(val)) {}
    String(long val) : std::string(std::to_string(val)) {}
    String(unsigned long val) : std::string(std::to_string(val)) {}
    void reserve(size_t n) { std::string::reserve(n); }
    const char* c_str() const { return std::string::c_str(); }
};
#endif

enum class AppLogLevel : uint8_t {
    LOG_ERROR = 1, // DE: Stufe 1: Fehler (Default) / EN: Level 1: Error (Default)
    LOG_DEBUG = 2, // DE: Stufe 2: Debug (wesentliche Infos) / EN: Level 2: Debug (essential events)
    LOG_INFO  = 3  // DE: Stufe 3: Info (vollständige Details) / EN: Level 3: Info (verbose details)
};

struct LogEntry {
    uint32_t id;
    uint32_t timestampMs;
    AppLogLevel level;
    char message[140];
};

class AppLogger {
public:
    static AppLogger& getInstance();

    // DE: Aktives Log-Level setzen / EN: Set active log level (1=ERROR, 2=DEBUG, 3=INFO)
    void setLevel(AppLogLevel level);
    AppLogLevel getLevel() const { return _currentLevel; }
    const char* getLevelName() const;
    size_t getCount() const { return _count; }

    // DE: Formatierte Log-Ausgabe (printf-Stil) / EN: Log printf-style formatted message
    void log(AppLogLevel level, const char* format, ...) __attribute__((format(printf, 3, 4)));

    // DE: Logs als JSON für WebUI-Streaming / EN: Returns logs as JSON string for WebUI
    String getLogsJson(uint32_t sinceId = 0) const;

    // DE: Speicherpuffer leeren / EN: Clear in-memory log buffer
    void clear();

    static const char* levelToString(AppLogLevel level);

private:
    AppLogger();
    AppLogLevel _currentLevel;

    static constexpr size_t MAX_LOG_ENTRIES = 100;
    LogEntry _entries[MAX_LOG_ENTRIES];
    size_t _head;
    size_t _count;
    uint32_t _nextId;
};

// DE: Komfort-Logging-Makros / EN: Convenience logging macros
#define LOG_E(fmt, ...) AppLogger::getInstance().log(AppLogLevel::LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_D(fmt, ...) AppLogger::getInstance().log(AppLogLevel::LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_I(fmt, ...) AppLogger::getInstance().log(AppLogLevel::LOG_INFO,  fmt, ##__VA_ARGS__)
