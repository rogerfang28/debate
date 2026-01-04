#pragma once
#include <string>
#include "httplib.h"

// Helper to extract user_id from cookies
class parseCookie {
public:
    static int extractUserIdFromCookies(const httplib::Request& req);
    static std::string extractUsernameFromCookies(const httplib::Request& req);
    static void setCookieUsername(httplib::Response& res, std::string username);
    static void setCookieUserId(httplib::Response& res, int user_id);
    static void clearAuthCookies(httplib::Response& res);
};