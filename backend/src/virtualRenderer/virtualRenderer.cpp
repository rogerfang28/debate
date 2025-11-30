#include "virtualRenderer.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "../debateModerator/DebateModerator.h"
#include "./LayoutGenerator/pageGenerator.h"
#include "./ClientMessageHandler/ClientMessageParser.h"
#include "./LayoutGenerator/LayoutGenerator.h"
// #include "../debate/main/EventHandler.h"
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

ui::Page VirtualRenderer::handleClientMessage(const client_message::ClientMessage& client_message, const std::string& user) {

    // translate client_message into debate event
    debate_event::DebateEvent evt = ClientMessageParser::parseMessage(client_message, user);

    // I NEED TO CALL THE DEBATE BACKEND SOMEHOW FROM HERE
    // BackendCommunicator backend("localhost", 8080);
    
    moderator_to_vr::ModeratorToVRMessage info;
    DebateModerator moderator;
    info = moderator.handleRequest(user, evt);
    // backend.sendEvent(evt, info);

    // parse the info and create a page
    ui::Page page = LayoutGenerator::generateLayout(info, user);

    // send back the page to handler
    return page;
}

std::string VirtualRenderer::extractUserFromCookies(const httplib::Request& req) {
    auto cookie_header = req.get_header_value("Cookie");
    std::cout << "[Auth] Cookie header: '" << cookie_header << "'\n";
    
    if (cookie_header.empty()) {
        std::cout << "[Auth] No cookie found, returning 'guest'\n";
        return "guest";
    }

    // Parse cookies looking for "user=<username>"
    std::string prefix = "user=";
    size_t pos = cookie_header.find(prefix);
    if (pos == std::string::npos) {
        std::cout << "[Auth] No 'user=' cookie found, returning 'guest'\n";
        return "guest";
    }

    size_t start = pos + prefix.length();
    size_t end = cookie_header.find(';', start);
    std::string user = (end == std::string::npos) 
        ? cookie_header.substr(start)
        : cookie_header.substr(start, end - start);

    std::cout << "[Auth] Extracted user from cookie: '" << user << "'\n";
    return user;
}