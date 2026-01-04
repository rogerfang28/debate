// * done
#include "pathUtils.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace utils {

    std::filesystem::path getExeDir() {
        #ifdef _WIN32
            wchar_t buffer[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            if (len == 0) {
                std::cerr << "[PathUtils][ERR] GetModuleFileNameW failed\n";
                return std::filesystem::current_path();
            }
            std::filesystem::path exePath(buffer);
            return exePath.parent_path();
        #else
            // For Unix-like systems, you'd implement this differently
            return std::filesystem::current_path();
        #endif
    }

    std::string getDatabasePath() {
        std::filesystem::path exeDir = getExeDir();
        // Navigate from exe location (usually backend/build/bin/) to backend/debates.sqlite3
        std::filesystem::path dbPath = exeDir / ".." / ".." / "debates.sqlite3";
        dbPath = std::filesystem::weakly_canonical(dbPath);
        // std::cerr << "[PathUtils] Database path resolved to: " << dbPath.string() << std::endl;
        return dbPath.string();
    }

}