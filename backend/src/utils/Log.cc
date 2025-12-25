#include "Log.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

// Initialize static member
LogLevel Log::currentLevel = LogLevel::Error;

void Log::init(LogLevel level) {
    currentLevel = level;
}

static std::string getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &tm);
    
    std::string result = buffer;
    result += "." + std::to_string(ms.count());
    return result;
}

void Log::error(const std::string& msg) {
    std::cerr << "[" << getTimestamp() << "] [ERROR] " << msg << "\n";
}

void Log::warn(const std::string& msg) {
    if (currentLevel >= LogLevel::Warn) {
        std::cerr << "[" << getTimestamp() << "] [WARN] " << msg << "\n";
    }
}

void Log::info(const std::string& msg) {
    if (currentLevel >= LogLevel::Info) {
        std::cout << "[" << getTimestamp() << "] [INFO] " << msg << "\n";
    }
}

void Log::debug(const std::string& msg) {
    if (currentLevel >= LogLevel::Debug) {
        std::cout << "[" << getTimestamp() << "] [DEBUG] " << msg << "\n";
    }
}
