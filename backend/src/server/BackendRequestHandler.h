#pragma once

#include <string>
#include <map>
#include "httplib.h"

class BackendRequestHandler {
public:
    BackendRequestHandler();
    ~BackendRequestHandler();

    std::string handlePostRequest(const httplib::Request& req, httplib::Response& res);
};