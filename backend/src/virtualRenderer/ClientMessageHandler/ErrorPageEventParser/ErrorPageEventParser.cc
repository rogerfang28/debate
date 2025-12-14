#include "ErrorPageEventParser.h"
#include <iostream>

debate_event::DebateEvent ErrorPageEventParser::ParseErrorPageEvent(
    const std::string& componentId,
    const std::string& eventType,
    const std::string& user,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    
    if (componentId == "goHomeButton" && eventType == "onClick") {
        event.set_type(debate_event::GO_HOME);
        auto* goHome = event.mutable_go_home();
        goHome->set_user_id(user);
        std::cout << "  GO_HOME from error page for user: " << user << "\n";
    } else {
        std::cerr << "[ErrorPageEvent] Unknown component/event combination on error page: " 
                  << componentId << "/" << eventType << "\n";
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}