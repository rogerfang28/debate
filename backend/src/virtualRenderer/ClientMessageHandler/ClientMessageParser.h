#pragma once

#include <string>
#include "client_message.pb.h"
#include "debate_event.pb.h"

class ClientMessageParser {
public:
    ClientMessageParser() = default;
    ~ClientMessageParser() = default;

    // Parse raw binary data into a ClientMessage protobuf
    static debate_event::DebateEvent parseMessage(const client_message::ClientMessage& message, const int& user_id);
    static void timestampEvent(debate_event::DebateEvent& event);
};