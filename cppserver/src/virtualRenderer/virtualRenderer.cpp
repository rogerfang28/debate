#include "virtualRenderer.h"
#include "../../../src/gen/cpp/page.pb.h"
#include "../../../src/gen/cpp/event.pb.h" // old
#include "../../../src/gen/cpp/eventData.pb.h"
#include "../../../src/gen/cpp/clientMessage.pb.h"
#include "./pageGenerator.h"
#include "../debate/main/EventHandler.h"
#include "../debate/main/OldEventHandler.h" // old
#include "../server/httplib.h"
#include <iostream>
using namespace std;

// Constructor
VirtualRenderer::VirtualRenderer() {
    cout << "VirtualRenderer initialized.\n";
}

// Destructor
VirtualRenderer::~VirtualRenderer() {
    cout << "VirtualRenderer destroyed.\n"; // probably never called
}

// Handle GET requests
void VirtualRenderer::handleGetRequest(const httplib::Request& req, httplib::Response& res) {
    string user = "defaultUser"; // later: parse from req.query_string
    // string page_bin = generateDebateClaimPage(user,"A","A");
    string page_bin = generatePage(user);

    if (page_bin.empty()) {
        res.status = 500;
        res.set_content("Failed to generate page\n", "text/plain");
        return;
    }

    res.set_content(page_bin, "application/x-protobuf");
    cout << "GET / served generated page for user=" << user
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
    OldEventHandler oldhandler(move(evt));
    oldhandler.handleEvent("defaultUser");

    res.status = 204; // No Content
    std::cout << "POST / handled debate.UIEvent\n";
}

void VirtualRenderer::handlePostEvent(const httplib::Request& req, httplib::Response& res) {
    if (req.get_header_value("Content-Type") != "application/x-protobuf") {
        res.status = 415;
        res.set_content("Unsupported Media Type: expected application/x-protobuf\n", "text/plain");
        return;
    }

    eventData::EventData evt;
    if (!evt.ParseFromArray(req.body.data(), (int)req.body.size())) {
        res.status = 400;
        res.set_content("Bad Request: failed to parse eventData.EventData\n", "text/plain");
        return;
    }
    EventHandler handler;
    handler.handleEvent(evt, "defaultUser");
}

void VirtualRenderer::handlePostClientMessage(const httplib::Request& req, httplib::Response& res) {
    if (req.get_header_value("Content-Type") != "application/x-protobuf") {
        res.status = 415;
        res.set_content("Unsupported Media Type: expected application/x-protobuf\n", "text/plain");
        return;
    }

    clientMessage::ClientMessage msg;
    if (!msg.ParseFromArray(req.body.data(), (int)req.body.size())) {
        res.status = 400;
        res.set_content("Bad Request: failed to parse clientMessage.ClientMessage\n", "text/plain");
        return;
    }

    EventHandler handler;
    handler.handleClientMessage(msg, "defaultUser");

    res.status = 204; // No Content
}