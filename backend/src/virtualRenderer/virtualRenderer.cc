#include "virtualRenderer.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "../debateModerator/DebateModerator.h"
#include "./ClientMessageHandler/ClientMessageParser.h"
#include "./LayoutGenerator/LayoutGenerator.h"
// #include "../debate/main/EventHandler.h"
#include "../server/httplib.h"
#include <iostream>
#include "../utils/Log.h"
#include "./utils/parseCookie.h"

// Constructor
VirtualRenderer::VirtualRenderer() {
    Log::debug("VirtualRenderer initialized.");
}

// Destructor
VirtualRenderer::~VirtualRenderer() {
    Log::debug("VirtualRenderer destroyed."); // probably never called
}

ui::Page VirtualRenderer::handleClientMessage(const client_message::ClientMessage& client_message, const std::string& cookie_header) {
    
    // translate client_message into debate event
    Log::debug("[VirtualRenderer] Handling ClientMessage, cookie: " + cookie_header);
    int user_id = parseCookie::extractUserIdFromCookies(cookie_header);
    debate_event::DebateEvent evt = ClientMessageParser::parseMessage(client_message, user_id);
    // I NEED TO CALL THE DEBATE BACKEND SOMEHOW FROM HERE
    // BackendCommunicator backend("localhost", 8080);
    // ! no server call for now, backend and virtual renderer are on the same backend
    
    moderator_to_vr::ModeratorToVRMessage info;
    // DebateModerator moderator;
    info = moderator.handleRequest(evt);
    // backend.sendEvent(evt, info);

    // parse the info and create a page
    ui::Page page = LayoutGenerator::generateLayout(info);
    // send back the page to handler
    return page;
}