#include "EnterDebateHandler.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../database/handlers/DebateDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include <iostream>
#include <vector>

bool EnterDebateHandler::EnterDebate(const std::string& debateId, const std::string& user) {
    std::cout << "[EnterDebate] User " << user << " entering debate with id: " << debateId << std::endl;

    DebateDatabaseHandler debateDbHandler(utils::getDatabasePath());
    std::vector<uint8_t> debateData = debateDbHandler.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateProto.ParseFromArray(debateData.data(), static_cast<int>(debateData.size()))) {
        std::cerr << "[EnterDebate][ERR] Failed to parse debate protobuf for debate ID " << debateId << std::endl;
        return false;
    }
    std::string rootClaimId = debateProto.root_claim_id();

    UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);

    user::User userProto;
    if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
        std::cerr << "[EnterDebate][ERR] Failed to parse user protobuf for " << user << std::endl;
        return false;
    }

    userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_DEBATING);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_root_claim()->set_id(rootClaimId);
    user_engagement::ClaimInfo currentClaim;
    currentClaim.set_id(rootClaimId);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->CopyFrom(currentClaim);
    
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
}
