#pragma once
#include <string>

// Helper to extract user_id from cookies
class parseCookie {
public:
    static int extractUserIdFromCookies(const std::string& cookie_header);
};