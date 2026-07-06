#pragma once

#include "debate_event.pb.h"
#include "client_message.pb.h"

class DebatePageEventParser {
public:
    static debate_event::DebateEvent ParseDebatePageEvent(
        const std::string& componentId,
        const std::string& eventType,
        const int& user_id,
        const client_message::ClientMessage& message
    );
};