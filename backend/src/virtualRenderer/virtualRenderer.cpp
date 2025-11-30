#include "virtualRenderer.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "../debate/DebateModerator.h"
#include "./pageGenerator.h"
#include "./ClientMessageHandler/ClientMessageParser.h"
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

// Handle GET requests
void VirtualRenderer::handleGetRequest(const httplib::Request& req, httplib::Response& res) {
    std::string user = extractUserFromCookies(req);
    std::cout << "[GET] User determined: '" << user << "'\n";
    
    // If no user cookie, show sign-in page
    if (user == "guest") {
        std::cout << "[GET] Serving sign-in page...\n";
        std::string page_bin = generateSignInPage();
        std::cout << "[GET] Sign-in page size: " << page_bin.size() << " bytes\n";
        res.set_content(page_bin, "application/x-protobuf");
        std::cout << "[GET] Served sign-in page (no user cookie)\n";
        return;
    }
    
    std::cout << "[GET] Serving authenticated page for user: " << user << "\n";
    std::string page_bin = generatePage(user);
    res.set_content(page_bin, "application/x-protobuf");
    std::cout << "[GET] Served page for user=" << user
            << " (" << page_bin.size() << " bytes)\n";
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
    ui::Page page;// = LayoutGenerator::generateLayout(info, user);
    // testing purposes temp fix

    // send back the page to handler
    return page;

    // Pass translated Event to EventHandler 
    // THIS IS THE OLD EVENT
    // EventHandler handler;
    // handler.handleEvent(evt, user);
    // res.status = 204;  // No Content
}