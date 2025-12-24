#pragma once

#include <string>
#include <map>
#include "httplib.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"

class MiddleendRequestHandler {
public:
    MiddleendRequestHandler() = default;
    ~MiddleendRequestHandler() = default;

    // Handle POST request
    void handleRequest(const httplib::Request& req, httplib::Response& res);
private:
    // Helper methods can be added here
    std::string extractUserFromCookies(const httplib::Request& req);
    ui::Page createLoginPage();
    void log(const std::string& user, const client_message::ClientMessage& msg);
    bool validateAuth(client_message::ClientMessage& msg, httplib::Response& res, const std::string& user);
};