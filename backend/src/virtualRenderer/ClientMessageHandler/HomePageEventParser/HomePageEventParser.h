#pragma once

#include "debate_event.pb.h"
#include "client_message.pb.h"

class HomePageEventParser {
public:
    static debate_event::DebateEvent ParseHomePageEvent(
        const std::string& componentId,
        const std::string& eventType,
        const client_message::ClientMessage& message
    );
};