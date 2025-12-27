#include "GoHomeHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include <iostream>
#include "../../../utils/Log.h"

bool GoHomeHandler::GoHome(const std::string& user, DebateWrapper& debateWrapper) {
    Log::debug("[GoHome] User " + user + " going home");
    
    try { 
        // Get current user protobuf
        user::User userProto = debateWrapper.getUserProtobufByUsername(user);
        
        // Update user state to NONE and clear debate topic id
        // userProto.set_state(user::NONE);
        userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_NONE);
        // set none info
        userProto.mutable_engagement()->mutable_none_info();
        // userProto.set_debate_topic_id("");
        
        // Serialize and save back to database
        std::vector<uint8_t> updatedData(userProto.ByteSizeLong());
        userProto.SerializeToArray(updatedData.data(), updatedData.size());
        
        debateWrapper.updateUserProtobuf(user, updatedData);
        
        return true;
    } catch (const std::exception& e) {
        Log::error("[GoHome][ERR] Exception: " + std::string(e.what()));
        return false;
    }
}