#pragma once

#include <string>
#include "../../../../src/gen/cpp/client_message.pb.h"
#include "../../../../src/gen/cpp/debate_event.pb.h"

class ClientMessageParser {
public:
    ClientMessageParser() = default;
    ~ClientMessageParser() = default;

    // Parse raw binary data into a ClientMessage protobuf
    debate_event::DebateEvent parseMessage(client_message::ClientMessage& message, std::string user);
};