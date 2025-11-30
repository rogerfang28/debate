#pragma once

#include <string>
#include <map>
#include "httplib.h"

class MiddleendRequestHandler {
public:
    MiddleendRequestHandler() = default;
    ~MiddleendRequestHandler() = default;

    // Handle POST request
    void handleRequest(const httplib::Request& req, httplib::Response& res);
private:
    // Helper methods can be added here
    std::string extractUserFromCookies(const httplib::Request& req);
};