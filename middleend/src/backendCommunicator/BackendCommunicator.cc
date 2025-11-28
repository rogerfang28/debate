// BackendCommunicator.cpp
#include "BackendCommunicator.h"
#include <iostream>

BackendCommunicator::BackendCommunicator(const std::string& backendHost, int backendPort) {
    std::string target = backendHost + ":" + std::to_string(backendPort);
    std::cout << "[BackendCommunicator] gRPC target: " << target << std::endl;

    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    stub_ = debate_message_service::DebateMessageService::NewStub(channel);
}

bool BackendCommunicator::sendEvent(
    const debate_event::DebateEvent& event,
    moderator_to_vr::ModeratorToVRMessage& outMessage
) {
    grpc::ClientContext context;
    moderator_to_vr::ModeratorToVRMessage response;

    grpc::Status status = stub_->SendDebateEvent(&context, event, &response);

    if (!status.ok()) {
        std::cerr << "[BackendCommunicator] SendDebateEvent RPC failed: "
                  << status.error_code() << " - " << status.error_message() << "\n";
        return false;
    }

    outMessage = std::move(response);
    return true;
}
