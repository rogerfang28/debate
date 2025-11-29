#include "enterDebate.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include <iostream>

bool enterDebate(const std::string& user, const std::string& topicID) {
    // std::string newtopicID = "16"; // Temporary override for testing
    std::cout << "[EnterDebate] User " << user << " entering debate topic with id: " << topicID << std::endl;

    try {
        // Create database handler
        UserDatabaseHandler userDbHandler(utils::getDatabasePath());
        
        // Get current user protobuf
        std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
        if (userData.empty()) {
            std::cerr << "[EnterDebate][ERR] User " << user << " not found" << std::endl;
            return false;
        }
        
        // Parse user protobuf
        user::User userProto;
        if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
            std::cerr << "[EnterDebate][ERR] Failed to parse user protobuf for " << user << std::endl;
            return false;
        }
        
        // Update user state to DEBATING and set debate topic id
        userProto.set_state(user::DEBATING);
        userProto.set_debate_topic_id(topicID);
        
        // Serialize and save back to database
        std::vector<uint8_t> updatedData(userProto.ByteSizeLong());
        userProto.SerializeToArray(updatedData.data(), updatedData.size());
        
        bool success = userDbHandler.updateUserProtobuf(user, updatedData);
        
        if (success) {
            std::cout << "[EnterDebate] Successfully moved user " << user << " to topic " << topicID 
                      << " (state=DEBATING)" << std::endl;
        } else {
            std::cerr << "[EnterDebate][ERR] Failed to update user protobuf for " << user << std::endl;
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "[EnterDebate][ERR] Exception: " << e.what() << std::endl;
        return false;
    }
}
