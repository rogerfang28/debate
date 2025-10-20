#include "virtualRenderer.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/event.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "./pageGenerator.h"
#include "../debate/main/EventHandler.h"
#include "../server/httplib.h"
#include <iostream>

// Constructor
VirtualRenderer::VirtualRenderer() {
    std::cout << "VirtualRenderer initialized.\n";
}

// Destructor
VirtualRenderer::~VirtualRenderer() {
    std::cout << "VirtualRenderer destroyed.\n"; // probably never called
}

// Handle GET requests
void VirtualRenderer::handleGetRequest(const httplib::Request& req, httplib::Response& res) {
    std::string user = "defaultUser"; // later: parse from req.query_string
    std::string page_bin = generatePage(user);

    res.set_content(page_bin, "application/x-protobuf");
    std::cout << "GET / served generated page for user=" << user
            << " (" << page_bin.size() << " bytes)\n";
}

// Handle POST requests
void VirtualRenderer::handlePostRequest(const httplib::Request& req, httplib::Response& res) {
    if (req.get_header_value("Content-Type") != "application/x-protobuf") {
        res.status = 415;
        res.set_content("Unsupported Media Type: expected application/x-protobuf\n", "text/plain");
        return;
    }

    debate::UIEvent evt;
    if (!evt.ParseFromArray(req.body.data(), (int)req.body.size())) {
        res.status = 400;
        res.set_content("Bad Request: failed to parse debate.UIEvent\n", "text/plain");
        return;
    }

    // // Handle with EventHandler
    EventHandler handler(std::move(evt));
    handler.handleEvent("defaultUser");

    res.status = 204; // No Content
    std::cout << "POST / handled debate.UIEvent\n";
}

void VirtualRenderer::handleClientMessage(const httplib::Request& req, httplib::Response& res) {
    // Parse ClientMessage protobuf
    debate::ClientMessage client_message;
    if (!client_message.ParseFromString(req.body)) {
      std::cerr << "❌ Failed to parse ClientMessage\n";
      res.status = 400;
      res.set_content("Failed to parse ClientMessage", "text/plain");
      return;
    }

    // Pass to EventHandler for processing
    EventHandler handler;
    handler.handleClientMessage(client_message, "defaultUser");

    res.status = 204;  // No Content
}