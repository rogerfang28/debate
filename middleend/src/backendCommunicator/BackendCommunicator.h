// BackendCommunicator.h
#pragma once

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "../../../src/gen/cpp/DebateMessageService.grpc.pb.h"
#include "../../../src/gen/cpp/DebateMessageService.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"

class BackendCommunicator {
public:
    BackendCommunicator(const std::string& backendHost, int backendPort);

    // Sends a DebateEvent and receives a ModeratorToVRMessage
    bool sendEvent(const debate_event::DebateEvent& event,
                   moderator_to_vr::ModeratorToVRMessage& outMessage);

private:
    std::unique_ptr<debate_message_service::DebateMessageService::Stub> stub_;
};
