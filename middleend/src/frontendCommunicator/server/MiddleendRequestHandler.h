#pragma once

#include <string>
#include <map>
#include "httplib.h"

class MiddleendRequestHandler {
public:
    MiddleendRequestHandler();
    ~MiddleendRequestHandler();

    // Handle POST request
    std::string handleRequest(const httplib::Request& req, httplib::Response& res);
private:
    // Helper methods can be added here
    std::string extractUserFromCookies(const httplib::Request& req);
};