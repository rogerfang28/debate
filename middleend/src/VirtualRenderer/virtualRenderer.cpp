#include "virtualRenderer.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "./pageGenerator.h"
#include "ClientMessageHandler/ClientMessageParser.h"
#include "../BackendCommunicator/BackendCommunicator.h"
#include "./layoutGenerator/LayoutGenerator.h"
#include "../../../backend/src/debate/DebateModerator.h"
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