#pragma once

#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace utils {
    // Get the directory where the executable is located
    std::filesystem::path getExeDir();
    
    // Get the path to the main database file in cppserver directory
    std::string getDatabasePath();
}