// * done
#include "pathUtils.h"
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
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
            char buffer[4096];
            ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (len <= 0) {
                std::cerr << "[PathUtils][ERR] readlink(/proc/self/exe) failed\n";
                return std::filesystem::current_path();
            }
            buffer[len] = '\0';
            return std::filesystem::path(buffer).parent_path();
        #endif
    }

    std::string getDatabasePath() {
        if (const char* envPath = std::getenv("DB_PATH"); envPath && envPath[0] != '\0') {
            std::filesystem::path configured = std::filesystem::path(envPath);
            if (configured.is_relative()) {
                configured = std::filesystem::current_path() / configured;
            }
            return configured.lexically_normal().string();
        }

        std::filesystem::path exeDir = getExeDir();
        // Navigate from exe location (usually backend/build/bin/) to backend/debates.sqlite3
        std::filesystem::path dbPath = exeDir / ".." / ".." / "debates.sqlite3";
        dbPath = std::filesystem::weakly_canonical(dbPath);
        // std::cerr << "[PathUtils] Database path resolved to: " << dbPath.string() << std::endl;
        return dbPath.string();
    }

}