#include "Log.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>

// Initialize static members
LogLevel Log::currentLevel = LogLevel::Debug;
std::mutex Log::logMutex;  // define the mutex

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
    
    std::ostringstream oss;
    oss << buffer << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

void Log::error(const std::string& msg) {
    if (currentLevel < LogLevel::Error) return;

    std::ostringstream oss;
    oss << "\033[31m"
        << "[" << getTimestamp() << "] [ERROR] " << msg
        << "\033[0m\n";

    std::lock_guard<std::mutex> lock(logMutex);
    std::cerr << oss.str();
}

void Log::warn(const std::string& msg) {
    if (currentLevel < LogLevel::Warn) return;

    std::ostringstream oss;
    oss << "\033[33m"
        << "[" << getTimestamp() << "] [WARN ] " << msg
        << "\033[0m\n";

    std::lock_guard<std::mutex> lock(logMutex);
    std::cerr << oss.str();
}

void Log::info(const std::string& msg) {
    if (currentLevel < LogLevel::Info) return;

    std::ostringstream oss;
    oss << "\033[36m"
        << "[" << getTimestamp() << "] [INFO ] " << msg
        << "\033[0m\n";

    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << oss.str();
}

void Log::debug(const std::string& msg) {
    if (currentLevel < LogLevel::Debug) return;

    std::ostringstream oss;
    oss << "\033[34m"
        << "[" << getTimestamp() << "] [DEBUG] " << msg
        << "\033[0m\n";

    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << oss.str();
}
