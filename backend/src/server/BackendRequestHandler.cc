// handles get and post requests and calls debate moderator
#include "BackendRequestHandler.h"
#include "../debate/DebateModerator.h"

#include "../../../src/gen/cpp/debate_event.pb.h"

std::string handleGetRequest(const httplib::Request& req, httplib::Response& res) {
    // parse the reqest and call event handler
    
}

std::string handlePostRequest(const httplib::Request& req, httplib::Response& res) {


}