#include <grpcpp/grpcpp.h>

#include "../../../src/gen/cpp/DebateMessageService.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using debate_message_service::DebateMessageService;
using debate_event::DebateEvent;
using moderator_to_vr::ModeratorToVRMessage;

class DebateMessageServiceImpl final : public DebateMessageService::Service {
public:
    Status SendDebateEvent(
        ServerContext* context,
        const DebateEvent* request,
        ModeratorToVRMessage* response
    ) override {
        // Fill response however you want
        // response->set_title("Example");

        return Status::OK;
    }
};

int main() {
    std::string addr = "0.0.0.0:8080";
    DebateMessageServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server = builder.BuildAndStart();
    server->Wait();
    return 0;
}
