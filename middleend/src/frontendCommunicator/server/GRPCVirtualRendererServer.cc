#include <grpcpp/grpcpp.h>

#include "../../../src/gen/cpp/ClientMessageService.grpc.pb.h"
#include "../../../src/gen/cpp/ClientMessageService.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using client_message_service::ClientMessageService;
using client_message::ClientMessage;
using ui::Page;

class ClientMessageServiceImpl final : public ClientMessageService::Service {
public:
    Status SendClientMessage(
        ServerContext* context,
        const ClientMessage* request,
        Page* response
    ) override {
        // Fill response however you want
        // response->set_title("Example");
        

        return Status::OK;
    }
};

int main() {
    std::string addr = "0.0.0.0:5000";
    ClientMessageServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server = builder.BuildAndStart();
    server->Wait();
    return 0;
}
