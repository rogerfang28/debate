#include "parseCookie.h"
#include "../../utils/Log.h"

// Helper to extract user_id from cookies
int parseCookie::extractUserIdFromCookies(const httplib::Request& req) {
    auto cookie_header = req.get_header_value("Cookie");
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

std::string parseCookie::extractUsernameFromCookies(const httplib::Request& req) {
    auto cookie_header = req.get_header_value("Cookie");
    // std::cout << "[Auth] Cookie header: '" << cookie_header << "'\n";
    
    if (cookie_header.empty()) {
        Log::debug("[Auth] No cookie found, returning ''");
        return "";
    }

    // Parse cookies looking for "username=<name>"
    std::string prefix = "username=";
    size_t pos = cookie_header.find(prefix);
    if (pos == std::string::npos) {
        Log::debug("[Auth] No 'username=' cookie found, returning ''");
        return "";
    }

    size_t start = pos + prefix.length();
    size_t end = cookie_header.find(';', start);
    std::string username = (end == std::string::npos) 
        ? cookie_header.substr(start)
        : cookie_header.substr(start, end - start);

    Log::debug("[Auth] Extracted username from cookie: " + username);
    return username;
}

void parseCookie::setCookieUsername(httplib::Response& res, std::string username) {
    std::string cookie_value = "username=" + username + "; Path=/; HttpOnly";
    res.set_header("Set-Cookie", cookie_value);
}

void parseCookie::setCookieUserId(httplib::Response& res, int user_id) {
    std::string cookie_value = "user_id=" + std::to_string(user_id) + "; Path=/; HttpOnly";
    res.set_header("Set-Cookie", cookie_value);
}

void parseCookie::clearAuthCookies(httplib::Response& res) {
    std::string expired_cookie = "user_id=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly";
    res.set_header("Set-Cookie", expired_cookie);
    expired_cookie = "username=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly";
    res.set_header("Set-Cookie", expired_cookie);
}