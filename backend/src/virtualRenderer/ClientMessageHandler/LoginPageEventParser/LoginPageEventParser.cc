#include "LoginPageEventParser.h"
#include "../../../utils/Log.h"
#include "../../../utils/DemoMode.h"

debate_event::DebateEvent LoginPageEventParser::ParseLoginPageEvent(
    const std::string& componentId,
    const std::string& eventType,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    const bool isSubmitClick = (componentId == "submitButton" && eventType == "onClick");
    const bool isDemoClick = (componentId == "demoButton" && eventType == "onClick");

    if (isSubmitClick || isDemoClick) {
        Log::debug("  LOGIN event triggered.");
        event.set_type(debate_event::LOGIN);
        auto* loginEvent = event.mutable_login();

        if (demo_mode::kDemoEnabled) {
            loginEvent->set_username("demo");
            Log::debug("  Demo mode enabled. Username forced to: demo");
        }
        else {
            std::string username;
            const auto& pageData = message.page_data().components();
            for (const auto& comp : pageData) {
                if (comp.id() == "usernameInput") {
                    username = comp.value();
                    break;
                }
            }

            loginEvent->set_username(username);
            Log::debug("  Username: " + username);
        }
    } 
    else {
        Log::warn("  Unhandled LoginPage event for component: " + componentId + " with event type: " + eventType);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    return event;
}