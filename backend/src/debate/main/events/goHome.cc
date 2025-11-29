#include "goHome.h"
#include "../../UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include <iostream>

bool goHome(const std::string& user) {
    std::cout << "[GoHome] User " << user << " going home" << std::endl;
    
    try {
        // Create database handler
        UserDatabaseHandler userDbHandler(utils::getDatabasePath());
        
        // Get current user protobuf
        std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
        if (userData.empty()) {
            std::cerr << "[GoHome][ERR] User " << user << " not found" << std::endl;
            return false;
        }
        
        // Parse user protobuf
        user::User userProto;
        if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
            std::cerr << "[GoHome][ERR] Failed to parse user protobuf for " << user << std::endl;
            return false;
        }
        
        // Update user state to NONE and clear debate topic id
        userProto.set_state(user::NONE);
        userProto.set_debate_topic_id("");
        
        // Serialize and save back to database
        std::vector<uint8_t> updatedData(userProto.ByteSizeLong());
        userProto.SerializeToArray(updatedData.data(), updatedData.size());
        
        bool success = userDbHandler.updateUserProtobuf(user, updatedData);
        
        if (success) {
            std::cout << "[GoHome] Successfully moved user " << user << " to home (state=NONE)" << std::endl;
        } else {
            std::cerr << "[GoHome][ERR] Failed to update user protobuf for " << user << std::endl;
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "[GoHome][ERR] Exception: " << e.what() << std::endl;
        return false;
    }
}
