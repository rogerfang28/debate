// handles get and post requests and calls debate moderator
#include "BackendRequestHandler.h"
#include "../debate/DebateModerator.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../virtual-renderer-communicator/BuildPayload.h"
#include <iostream>

BackendRequestHandler::BackendRequestHandler() {
    std::cout << "[BackendRequestHandler] Initialized" << std::endl;
}

BackendRequestHandler::~BackendRequestHandler() {
    std::cout << "[BackendRequestHandler] Destroyed" << std::endl;
}

std::string BackendRequestHandler::handlePostRequest(const httplib::Request& req, httplib::Response& res) {
    // Extract username from custom header
    std::string username = "guest";
    if (req.has_header("X-User")) {
        username = req.get_header_value("X-User");
    }
    
    std::cout << "[BackendRequestHandler] POST request from user: " << username << std::endl;
    
    // Parse DebateEvent from body
    debate_event::DebateEvent event;
    if (!event.ParseFromString(req.body)) {
        std::cerr << "[BackendRequestHandler] Failed to parse DebateEvent from request body" << std::endl;
        res.status = 400;
        res.set_content("Invalid protobuf", "text/plain");
        return "";
    }
    
    std::cout << "[BackendRequestHandler] Parsed event type: " << event.type() << std::endl;
    
    // Call DebateModerator
    DebateModerator moderator;
    moderator_to_vr::ModeratorToVRMessage vr_msg;
    vr_msg = moderator.handleRequest(username, event);

    // Serialize protobuf and send back
    std::string serialized = vr_msg.SerializeAsString();
    res.set_content(serialized, "application/x-protobuf");
    return "";
}