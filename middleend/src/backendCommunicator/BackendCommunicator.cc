// send backend a debateEvent
// receives renderInfo

#include "BackendCommunicator.h"
#include <iostream>
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"

BackendCommunicator::BackendCommunicator(const std::string& backendHost, int backendPort)
    : backendHost(backendHost), backendPort(backendPort) {
    std::cout << "[BackendCommunicator] Initialized with backend at " 
              << backendHost <<":" << backendPort << std::endl;
}

bool BackendCommunicator::getUserInfo(const std::string& username, moderator_to_vr::ModeratorToVRMessage& outMessage) {
    std::string response = sendGetRequest("/", username);
    
    if (response.empty()) {
        std::cerr << "[BackendCommunicator] Empty response from GET request" << std::endl;
        return false;
    }
    
    if (!outMessage.ParseFromString(response)) {
        std::cerr << "[BackendCommunicator] Failed to parse ModeratorToVRMessage from response" << std::endl;
        return false;
    }
    
    std::cout << "[BackendCommunicator] Successfully parsed response message" << std::endl;
    return true;
}

bool BackendCommunicator::sendEvent(const std::string& username, const debate_event::DebateEvent& event,
                                     moderator_to_vr::ModeratorToVRMessage& outMessage) {
    std::string serialized = event.SerializeAsString();
    std::string response = sendPostRequest("/", username, serialized);
    
    if (response.empty()) {
        std::cerr << "[BackendCommunicator] Empty response from POST request" << std::endl;
        return false;
    }
    
    if (!outMessage.ParseFromString(response)) {
        std::cerr << "[BackendCommunicator] Failed to parse ModeratorToVRMessage from response" << std::endl;
        return false;
    }
    
    std::cout << "[BackendCommunicator] Successfully parsed response message" << std::endl;
    return true;
}

std::string BackendCommunicator::sendGetRequest(const std::string& path, const std::string& username) {
    httplib::Client client(backendHost, backendPort);
    
    // Set a timeout (optional)
    client.set_read_timeout(5, 0); // 5 seconds
    
    // Add username as query parameter
    std::string fullPath = path;
    if (!username.empty()) {
        fullPath += "?user=" + username;
    }
    
    auto res = client.Get(fullPath);
    
    if (res && res->status == 200) {
        std::cout << "[BackendCommunicator] GET " << fullPath << " succeeded" << std::endl;
        return res->body;
    } else {
        std::cerr << "[BackendCommunicator] GET " << fullPath << " failed";
        if (res) {
            std::cerr << " with status: " << res->status;
        }
        std::cerr << std::endl;
        return "";
    }
}

std::string BackendCommunicator::sendPostRequest(const std::string& path, 
                                                  const std::string& username,
                                                  const std::string& body) {
    httplib::Client client(backendHost, backendPort);
    
    // Set a timeout (optional)
    client.set_read_timeout(5, 0); // 5 seconds
    
    // Set custom headers
    httplib::Headers headers = {
        {"X-User", username},
        {"Content-Type", "application/x-protobuf"}
    };
    
    auto res = client.Post(path, headers, body, "application/x-protobuf");
    
    if (res && res->status == 200) {
        std::cout << "[BackendCommunicator] POST " << path << " succeeded" << std::endl;
        return res->body;
    } else {
        std::cerr << "[BackendCommunicator] POST " << path << " failed";
        if (res) {
            std::cerr << " with status: " << res->status;
        }
        std::cerr << std::endl;
        return "";
    }
}