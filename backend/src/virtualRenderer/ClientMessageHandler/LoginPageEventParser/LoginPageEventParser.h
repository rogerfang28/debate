#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../../../src/gen/cpp/client_message.pb.h"

class LoginPageEventParser {
public:
    static debate_event::DebateEvent ParseLoginPageEvent(
        const std::string& componentId,
        const std::string& eventType,
        // const std::string& user,
        const client_message::ClientMessage& message
    );
};