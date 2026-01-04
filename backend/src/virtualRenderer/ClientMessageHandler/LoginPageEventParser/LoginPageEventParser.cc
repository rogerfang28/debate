#include "LoginPageEventParser.h"
#include "../../../utils/Log.h"

debate_event::DebateEvent LoginPageEventParser::ParseLoginPageEvent(
    const std::string& componentId,
    const std::string& eventType,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    // * Handle login event, for now only takes username
    if (componentId == "submitButton" && eventType == "onClick") {
        Log::debug("  LOGIN event triggered.");
        event.set_type(debate_event::LOGIN);
        auto* loginEvent = event.mutable_login();
        const auto& pageData = message.page_data().components();
        for (const auto& comp : pageData) { 
            if (comp.id() == "usernameInput") {
                loginEvent->set_username(comp.value());
                Log::debug("  Username: " + comp.value());
                break;
            }
        }
    } 
    else {
        Log::warn("  Unhandled LoginPage event for component: " + componentId + " with event type: " + eventType);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    return event;
}