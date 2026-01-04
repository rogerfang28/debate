#pragma once

#include <string>
#include <map>
#include "httplib.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "./virtualRenderer.h"

class MiddleendRequestHandler {
public:
    MiddleendRequestHandler();
    ~MiddleendRequestHandler() = default;

    // Handle POST request
    void handleRequest(const httplib::Request& req, httplib::Response& res);
private:
    // Helper methods can be added here
    int extractUserIdFromCookies(const httplib::Request& req);
    ui::Page createLoginPage();
    void log(int userId, const client_message::ClientMessage& msg);
    bool validateAuth(client_message::ClientMessage& msg, httplib::Response& res, int userId);
    VirtualRenderer renderer;
};