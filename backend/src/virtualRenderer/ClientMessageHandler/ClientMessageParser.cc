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
#include "../../utils/Log.h"

debate_event::DebateEvent ClientMessageParser::parseMessage(const client_message::ClientMessage& message, const std::string& user) {
    debate_event::DebateEvent event;
    timestampEvent(event); // set timestamp and id

    // check to see if there is nothing actually happening, like nothing clicked, it's basically no event
    if (message.component_id().empty() && message.event_type().empty()) {
        Log::debug("No action in ClientMessage, returning empty DebateEvent.");
        event.set_type(debate_event::NONE);
        return event;
    }

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
        event = DebatePageEventParser::ParseDebatePageEvent(componentId, eventType, user, message);
    } else if (pageId == "error") {
        event = ErrorPageEventParser::ParseErrorPageEvent(componentId, eventType, user, message);
    } else {
        Log::error("Unknown page ID: " + pageId);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    event.set_user_id(user);

    Log::debug("  Event Type: " + std::to_string(event.type()));
    Log::debug("========================================\n");
    
    return event;
}

void ClientMessageParser::timestampEvent(debate_event::DebateEvent& event) {
    // Set common fields
    event.set_id("evt_" + std::to_string(std::time(nullptr)));
    
    // Set timestamp to current time
    auto* timestamp = event.mutable_occurred_at();
    auto now = std::chrono::system_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    timestamp->set_seconds(seconds);
}