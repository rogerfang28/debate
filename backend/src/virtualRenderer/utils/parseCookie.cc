#include "parseCookie.h"
#include "../../utils/Log.h"

// Helper to extract user_id from cookies
int parseCookie::extractUserIdFromCookies(const std::string& cookie_header) {
    // auto cookie_header = req.get_header_value("Cookie");
    // std::cout << "[Auth] Cookie header: '" << cookie_header << "'\n";
    
    if (cookie_header.empty()) {
        Log::debug("[Auth] No cookie found, returning 0 (guest)");
        return 0;
    }

    // Parse cookies looking for "user_id=<id>"
    std::string prefix = "user_id=";
    size_t pos = cookie_header.find(prefix);
    if (pos == std::string::npos) {
        Log::debug("[Auth] No 'user_id=' cookie found, returning 0 (guest)");
        return 0;
    }

    size_t start = pos + prefix.length();
    size_t end = cookie_header.find(';', start);
    std::string userIdStr = (end == std::string::npos) 
        ? cookie_header.substr(start)
        : cookie_header.substr(start, end - start);

    try {
        int userId = std::stoi(userIdStr);
        Log::debug("[Auth] Extracted user_id from cookie: " + std::to_string(userId));
        return userId;
    } catch (const std::exception& e) {
        Log::error("[Auth] Failed to parse user_id: " + userIdStr);
        return 0;
    }
}
