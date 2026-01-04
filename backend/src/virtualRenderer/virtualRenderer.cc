#include "virtualRenderer.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "../debateModerator/DebateModerator.h"
#include "./ClientMessageHandler/ClientMessageParser.h"
#include "./LayoutGenerator/LayoutGenerator.h"
#include "./LayoutGenerator/pages/loginPage/LoginPageGenerator.h"
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

ui::Page VirtualRenderer::handleClientMessage(const client_message::ClientMessage& client_message,const httplib::Request& req, httplib::Response& res) {
    
    // translate client_message into debate event
    int user_id = parseCookie::extractUserIdFromCookies(req);
    debate_event::DebateEvent evt = ClientMessageParser::parseMessage(client_message, user_id);
    // BackendCommunicator backend("localhost", 8080);
    // ! no server call for now, backend and virtual renderer are on the same backend
    
    // change cookies accordingly
    handleAuthEvents(evt, req, res);
    
    moderator_to_vr::ModeratorToVRMessage info;
    info = moderator.handleRequest(evt);
    // backend.sendEvent(evt, info);

    // parse user info to create layout based on it
    ui::Page page = LayoutGenerator::generateLayout(info);
    return page;
}



void VirtualRenderer::handleAuthEvents(debate_event::DebateEvent& evt, const httplib::Request& req, httplib::Response& res) {
    if (evt.type() == debate_event::LOGOUT) {
        Log::debug("[VirtualRenderer] Logout event detected, clearing req cookies.");
        parseCookie::clearAuthCookies(res);
    }
    else if (evt.type() == debate_event::LOGIN) {
        Log::debug("[VirtualRenderer] Login event detected, login the user with cookies.");
        int userId = moderator.getUserId(evt.login().username());
        // could return -1 for no user
        if (userId == -1) {
            Log::debug("[VirtualRenderer] User not found during login, creating new user.");
            userId = moderator.createUserIfNotExist(evt.login().username());
        }
        // this ensures a user exists
        parseCookie::setCookieUserId(res, userId);
        parseCookie::setCookieUsername(res, evt.login().username());
    }

    // add login info to the event
    evt.mutable_user()->set_user_id(parseCookie::extractUserIdFromCookies(req));
    evt.mutable_user()->set_username(parseCookie::extractUsernameFromCookies(req));
    if (evt.mutable_user()->username() != "" && evt.mutable_user()->user_id() != -1 && evt.mutable_user()->user_id() != 0)  { 
        // scuffed hardcode
        evt.mutable_user()->set_is_logged_in(true);
    } else {
        evt.mutable_user()->set_is_logged_in(false);
    }
}