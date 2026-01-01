#include "ClientMessageParser.h"
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include "../../../../src/gen/cpp/client_message.pb.h"
#include "../../../../src/gen/cpp/debate_event.pb.h"
#include "HomePageEventParser/HomePageEventParser.h"
#include "DebatePageEventParser/DebatePageEventParser.h"
#include "ErrorPageEventParser/ErrorPageEventParser.h"
#include "LoginPageEventParser/LoginPageEventParser.h"
#include "../../utils/Log.h"
#include "../utils/parseCookie.h"

debate_event::DebateEvent ClientMessageParser::parseMessage(const client_message::ClientMessage& message, const int& user_id) {
    debate_event::DebateEvent event;
    timestampEvent(event);

    // check to see if there is nothing actually happening, like nothing clicked, it's basically no event
    // this happens when they are just loading the page, not clicking and stuff
    if (message.component_id().empty() && message.event_type().empty()) {
        event.set_type(debate_event::NONE);
        return event;
    }

    event.mutable_user()->set_user_id(user_id); 
    // This should work even though the event for login wouldn't be able 
    // to process the event yet, because the frontened sends 2 separate messages
    // one for the login event, and one none event to load the page after login
    event.mutable_user()->set_is_logged_in(user_id != 0); // 0 is not logged in

    const std::string& componentId = message.component_id();
    const std::string& eventType = message.event_type();
    const std::string& pageId = message.page_data().page_id();
    
    Log::debug("\n--- Translating ClientMessage to DebateEvent ---");
    Log::debug("  Page ID: " + pageId);
    Log::debug("  Component ID: " + componentId);
    Log::debug("  Event Type: " + eventType);
    
    // Route based on page_id and delegate to specific parsers
    if (pageId == "home") {
        event = HomePageEventParser::ParseHomePageEvent(componentId, eventType, message);
    } else if (pageId == "debate") {
        event = DebatePageEventParser::ParseDebatePageEvent(componentId, eventType, user_id, message);
    } else if (pageId == "error") {
        event = ErrorPageEventParser::ParseErrorPageEvent(componentId, eventType, user_id, message);
    } else if (pageId == "login") {
        event = LoginPageEventParser::ParseLoginPageEvent(componentId, eventType, message);
    }
    else {
        Log::error("Unknown page ID: " + pageId);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}

void ClientMessageParser::timestampEvent(debate_event::DebateEvent& event) {
    auto* timestamp = event.mutable_occurred_at();
    auto now = std::chrono::system_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    timestamp->set_seconds(seconds);
}

