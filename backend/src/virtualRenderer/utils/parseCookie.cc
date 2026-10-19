#include "parseCookie.h"
#include "../../utils/Log.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

namespace {
bool envFlagEnabled(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return false;
    }

    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

bool isLocalOrigin(const std::string& value) {
    return value.find("localhost") != std::string::npos || value.find("127.0.0.1") != std::string::npos;
}

bool isHttpsRequest(const httplib::Request& req) {
    const std::string origin = req.get_header_value("Origin");
    const std::string host = req.get_header_value("Host");
    return origin.find("https://") != std::string::npos || host.find(":443") != std::string::npos;
}

std::string cookieAttributesFromRequest(const httplib::Request& req) {
    std::string sameSite = "Lax";
    bool secure = false;

    if (const char* mode = std::getenv("DEBATE_COOKIE_MODE"); mode != nullptr && mode[0] != '\0') {
        std::string normalized(mode);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (normalized == "hosted" || normalized == "production" || normalized == "https") {
            sameSite = "None";
            secure = true;
        }
    }
    else {
        const std::string origin = req.get_header_value("Origin");
        const std::string host = req.get_header_value("Host");
        const bool hostedRequest = (!origin.empty() && !isLocalOrigin(origin)) || (!host.empty() && !isLocalOrigin(host));

        if (hostedRequest) {
            // Only use Secure + SameSite=None for HTTPS
            if (isHttpsRequest(req)) {
                sameSite = "None";
                secure = true;
            } else {
                // HTTP hosted request — use Lax so cookies work over plain HTTP
                sameSite = "Lax";
                secure = false;
            }
        }
    }

    if (const char* sameSiteEnv = std::getenv("DEBATE_COOKIE_SAMESITE"); sameSiteEnv != nullptr && sameSiteEnv[0] != '\0') {
        sameSite = sameSiteEnv;
    }

    if (std::getenv("DEBATE_COOKIE_SECURE") != nullptr) {
        secure = envFlagEnabled("DEBATE_COOKIE_SECURE");
    }

    std::string attributes = "; Path=/; HttpOnly";
    if (!sameSite.empty()) {
        attributes += "; SameSite=" + sameSite;
    }
    if (secure) {
        attributes += "; Secure";
    }

    if (sameSite == "None" && !secure) {
        Log::info("[Auth] Cookie policy uses SameSite=None without Secure; browsers may reject it on modern builds.");
    }

    return attributes;
}
}

// Helper to extract user_id from cookies
int parseCookie::extractUserIdFromCookies(const httplib::Request& req) {
    auto cookie_header = req.get_header_value("Cookie");
    Log::info("[Auth] Cookie header: '" + cookie_header + "'");
    
    if (cookie_header.empty()) {
        Log::info("[Auth] No cookie found, returning 0 (guest)");
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

void parseCookie::setCookieUsername(const httplib::Request& req, httplib::Response& res, std::string username) {
    std::string cookie_value = "username=" + username + cookieAttributesFromRequest(req);
    res.set_header("Set-Cookie", cookie_value);
}

void parseCookie::setCookieUserId(const httplib::Request& req, httplib::Response& res, int user_id) {
    std::string cookie_value = "user_id=" + std::to_string(user_id) + cookieAttributesFromRequest(req);
    res.set_header("Set-Cookie", cookie_value);
}

void parseCookie::clearAuthCookies(const httplib::Request& req, httplib::Response& res) {
    const std::string attributes = cookieAttributesFromRequest(req);
    std::string expired_cookie = "user_id=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly";
    expired_cookie += attributes;
    res.set_header("Set-Cookie", expired_cookie);
    expired_cookie = "username=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly";
    expired_cookie += attributes;
    res.set_header("Set-Cookie", expired_cookie);
}