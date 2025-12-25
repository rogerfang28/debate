#include "GoHomeHandler.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include <iostream>
#include "../../../utils/Log.h"

bool GoHomeHandler::GoHome(const std::string& user) {
    Log::debug("[GoHome] User " + user + " going home");
    
    try {
        UserDatabaseHandler userDbHandler(utils::getDatabasePath());
        
        // Get current user protobuf
        std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
        if (userData.empty()) {
            Log::error("[GoHome][ERR] User " + user + " not found");
            return false;
        }
        
        // Parse user protobuf
        user::User userProto;
        if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
            Log::error("[GoHome][ERR] Failed to parse user protobuf for " + user);
            return false;
        }
        
        // Update user state to NONE and clear debate topic id
        // userProto.set_state(user::NONE);
        userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_NONE);
        // set none info
        userProto.mutable_engagement()->mutable_none_info();
        // userProto.set_debate_topic_id("");
        
        // Serialize and save back to database
        std::vector<uint8_t> updatedData(userProto.ByteSizeLong());
        userProto.SerializeToArray(updatedData.data(), updatedData.size());
        
        bool success = userDbHandler.updateUserProtobuf(user, updatedData);
        
        return success;
    } catch (const std::exception& e) {
        Log::error("[GoHome][ERR] Exception: " + std::string(e.what()));
        return false;
    }
}