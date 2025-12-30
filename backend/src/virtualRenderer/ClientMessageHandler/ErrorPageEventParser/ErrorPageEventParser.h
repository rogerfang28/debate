#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../../../src/gen/cpp/client_message.pb.h"

class ErrorPageEventParser {
public:
    static debate_event::DebateEvent ParseErrorPageEvent(
        const std::string& componentId,
        const std::string& eventType,
        const int& user_id,
        const client_message::ClientMessage& message
    );
};