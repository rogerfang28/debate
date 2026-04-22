#pragma once
#include <string>
#include "httplib.h"

// Helper to extract user_id from cookies
class parseCookie {
public:
    static int extractUserIdFromCookies(const httplib::Request& req);
    static std::string extractUsernameFromCookies(const httplib::Request& req);
    static void setCookieUsername(const httplib::Request& req, httplib::Response& res, std::string username);
    static void setCookieUserId(const httplib::Request& req, httplib::Response& res, int user_id);
    static void clearAuthCookies(const httplib::Request& req, httplib::Response& res);
};