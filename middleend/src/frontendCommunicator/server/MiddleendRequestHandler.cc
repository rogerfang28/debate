// has handle get or post or any request, calls virtual renderer
#include "MiddleendRequestHandler.h"

#include "../../../src/gen/cpp/client_message.pb.h"
#include "../../VirtualRenderer/virtualRenderer.h"

std::string handleGetRequest(const httplib::Request& req, httplib::Response& res) {
    // parse the reqest and call virtual renderer
    VirtualRenderer renderer;
    renderer.handleGetRequest(req, res);
}

std::string handlePostRequest(const httplib::Request& req, httplib::Response& res) {
    VirtualRenderer renderer;
    renderer.handleClientMessage(req, res);  
}