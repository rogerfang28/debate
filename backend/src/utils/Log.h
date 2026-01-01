#pragma once

#include <string>
#include <mutex>

enum class LogLevel {
    None = -1,
    Error = 0,
    Warn  = 1,
    Info  = 2,
    Debug = 3
};

class Log {
public:
    // Initialize the logger with a minimum log level
    static void init(LogLevel level);
    
    // Logging functions
    static void error(const std::string& msg);
    static void warn(const std::string& msg);
    static void info(const std::string& msg);
    static void debug(const std::string& msg);

private:
    static LogLevel currentLevel;
    static std::mutex logMutex;   // NEW: synchronize all log output
};
