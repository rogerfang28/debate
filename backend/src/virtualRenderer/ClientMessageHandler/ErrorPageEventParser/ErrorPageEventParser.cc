#include "ErrorPageEventParser.h"
#include <iostream>
#include "../../../utils/Log.h"

debate_event::DebateEvent ErrorPageEventParser::ParseErrorPageEvent(
    const std::string& componentId,
    const std::string& eventType,
    const int& user_id,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    
    if (componentId == "goHomeButton" && eventType == "onClick") {
        event.set_type(debate_event::GO_HOME);
        auto* goHome = event.mutable_go_home();
        Log::debug("  GO_HOME from error page for user: " + std::to_string(user_id));
    } else {
        Log::error("[ErrorPageEvent] Unknown component/event combination on error page: " 
                  + componentId + "/" + eventType);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}