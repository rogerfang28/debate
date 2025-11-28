#pragma once

#include <httplib.h>
#include <string>
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"

class BackendCommunicator {
public:
    BackendCommunicator(const std::string& backendHost = "localhost", int backendPort = 8080);
    
    // High-level API: Get user information from backend
    bool getUserInfo(const std::string& username, moderator_to_vr::ModeratorToVRMessage& outMessage);
    
    // High-level API: Send event and get response
    bool sendEvent(const std::string& username, const debate_event::DebateEvent& event, 
                   moderator_to_vr::ModeratorToVRMessage& outMessage);
    
    // Low-level API: Send GET request to backend with username
    std::string sendGetRequest(const std::string& path, const std::string& username);
    
    // Low-level API: Send POST request to backend with username and protobuf body
    std::string sendPostRequest(const std::string& path, const std::string& username, const std::string& body);
    
private:
    std::string backendHost;
    int backendPort;
};