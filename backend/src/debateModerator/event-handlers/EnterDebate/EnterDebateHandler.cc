#include "EnterDebateHandler.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include <iostream>

bool EnterDebateHandler::EnterDebate(const std::string& rootClaimId, const std::string& user) {
    // std::string newtopicID = "16"; // Temporary override for testing
    std::cout << "[EnterDebate] User " << user << " entering debate root claim with id: " << rootClaimId << std::endl;

    try {
        UserDatabaseHandler userDbHandler(utils::getDatabasePath());
        std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);

        user::User userProto;
        if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
            std::cerr << "[EnterDebate][ERR] Failed to parse user protobuf for " << user << std::endl;
            return false;
        }

        userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_DEBATING);
        userProto.mutable_engagement()->mutable_debating_info()->set_root_claim_id(rootClaimId);
        user_engagement::ClaimInfo currentClaim;
        currentClaim.set_id(rootClaimId);
        // currentClaim.set_sentence("Root claim for debate topic " + rootClaimId);
        // userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(); // root
        
        // userProto.mutable_engagement()->mutable_debating_info()->set_current_claim_id("0"); // root
        // userProto.set_debate_topic_id(topicID);
        
        // Serialize and save back to database
        std::vector<uint8_t> updatedData(userProto.ByteSizeLong());
        userProto.SerializeToArray(updatedData.data(), updatedData.size());
        
        bool success = userDbHandler.updateUserProtobuf(user, updatedData);
        
        if (success) {
            std::cout << "[EnterDebate] Successfully moved user " << user << " to topic " << rootClaimId 
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
