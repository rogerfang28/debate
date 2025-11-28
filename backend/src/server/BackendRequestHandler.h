#pragma once

#include <string>
#include <map>
#include "httplib.h"

class BackendRequestHandler {
public:
    BackendRequestHandler();
    ~BackendRequestHandler();

    // Handle GET request
    std::string handleGetRequest(const httplib::Request& req, httplib::Response& res);

    // Handle POST request
    std::string handlePostRequest(const httplib::Request& req, httplib::Response& res);
private:
    // Helper methods can be added here
};