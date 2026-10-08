#include "LoginPageEventParser.h"
#include "../../../utils/Log.h"
#include "../../../utils/DemoMode.h"
#include "../../../utils/AnonymousUser.h"

debate_event::DebateEvent LoginPageEventParser::ParseLoginPageEvent(
    const std::string& componentId,
    const std::string& eventType,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    const bool isSubmitClick = (componentId == "submitButton" && eventType == "onClick");
    const bool isDemoClick = (componentId == "demoButton" && eventType == "onClick");
    const bool isGuestClick = (componentId == "guestButton" && eventType == "onClick");
    const bool isGoogleClick = (componentId == "googleLoginButton" && eventType == "onGoogleLogin");

    if (isGoogleClick) {
        // Google Sign-In: extract the google_id_token from the pageData components
        Log::debug("  GOOGLE LOGIN event triggered.");
        event.set_type(debate_event::LOGIN);
        auto* loginEvent = event.mutable_login();

        // Find google_id_token in the pageData components
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "google_id_token") {
                loginEvent->set_google_id_token(comp.value());
                Log::debug("  Google ID token received (" + std::to_string(comp.value().size()) + " chars).");
                break;
            }
        }

        if (loginEvent->google_id_token().empty()) {
            Log::error("  Google login without token — falling back to unspecified event.");
            event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
        }
    }
    else if (isGuestClick) {
        Log::debug("  GUEST LOGIN event triggered.");
        event.set_type(debate_event::LOGIN);
        // The same click, sent by the client on its own at first load with a
        // visitorName it made up and kept: each first-time reader becomes
        // their own read-only visitor instead of sharing "guest". A name that
        // is not exactly the visitor shape is ignored.
        std::string username = "guest";
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "visitorName" &&
                anonymous_user::IsWellFormedVisitorName(comp.value())) {
                username = comp.value();
                break;
            }
        }
        event.mutable_login()->set_username(username);
    }
    else if (isSubmitClick || isDemoClick) {
        Log::debug("  LOGIN event triggered.");
        event.set_type(debate_event::LOGIN);
        auto* loginEvent = event.mutable_login();

        if (demo_mode::kDemoEnabled) {
            loginEvent->set_username("demo");
            Log::debug("  Demo mode enabled. Username forced to: demo");
        }
        else {
            std::string username;
            std::string password;
            const auto& pageData = message.page_data().components();
            for (const auto& comp : pageData) {
                if (comp.id() == "usernameInput") {
                    username = comp.value();
                } else if (comp.id() == "passwordInput") {
                    password = comp.value();
                }
            }

            loginEvent->set_username(username);
            loginEvent->set_password(password);
            Log::debug("  Username: " + username + " (password " +
                       (password.empty() ? "empty" : "provided") + ")");
        }
    }
    else {
        Log::warn("  Unhandled LoginPage event for component: " + componentId + " with event type: " + eventType);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    return event;
}