#pragma once

#include "debate_event.pb.h"
#include "client_message.pb.h"

class LoginPageEventParser {
public:
    // The header's and the sign-in modal's buttons. They are on every page,
    // so ClientMessageParser sends them here whatever page_id they came from.
    static bool IsSessionComponent(const std::string& componentId);
    static debate_event::DebateEvent ParseLoginPageEvent(
        const std::string& componentId,
        const std::string& eventType,
        // const std::string& user,
        const client_message::ClientMessage& message
    );
};