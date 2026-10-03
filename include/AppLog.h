#pragma once

/*
 * ==============================================================================
 * QUELLENNACHWEIS / SOURCE ATTRIBUTION:
 * ==============================================================================
 * AppLog: 3-stufiges Logging- und Ringspeichersystem für RocNet ESP32 Controller
 *
 * Stufen:
 *   1. LOG_ERROR = Nur kritische Fehler (DEFAULT)
 *   2. LOG_DEBUG = Nur wesentliche, essentielle Statusinfos (Weichen, Signale, Node)
 *   3. LOG_INFO  = Alle Details (Raw Telegramm-Hexdumps, Heartbeats, PWM-Details)
 *
 * Beinhaltet einen Ringspeicher für das WebUI-Live-Logging und REST-Endpoints.
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
    LOG_ERROR = 1, // Stufe 1: Fehler (Default)
    LOG_DEBUG = 2, // Stufe 2: Debug (nur wesentliche, essentielle Infos)
    LOG_INFO  = 3  // Stufe 3: Info (alles, was übrig blieb: Hexdumps, Heartbeats)
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

    // Set active log level (1=ERROR, 2=DEBUG, 3=INFO)
    void setLevel(AppLogLevel level);
    AppLogLevel getLevel() const { return _currentLevel; }
    const char* getLevelName() const;
    size_t getCount() const { return _count; }

    // Log a message formatted printf-style
    void log(AppLogLevel level, const char* format, ...) __attribute__((format(printf, 3, 4)));

    // Returns logs as JSON string for WebUI live streaming
    String getLogsJson(uint32_t sinceId = 0) const;

    // Clear memory buffer
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

// Convenience macros
#define LOG_E(fmt, ...) AppLogger::getInstance().log(AppLogLevel::LOG_ERROR, fmt, ##__VA_ARGS__)
#define LOG_D(fmt, ...) AppLogger::getInstance().log(AppLogLevel::LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_I(fmt, ...) AppLogger::getInstance().log(AppLogLevel::LOG_INFO,  fmt, ##__VA_ARGS__)
