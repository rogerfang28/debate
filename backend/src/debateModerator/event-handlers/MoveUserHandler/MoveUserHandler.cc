#include "MoveUserHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include "../../../utils/Log.h"
#include "../../../utils/pathUtils.h"
#include <iostream>
#include <vector>

bool MoveUserHandler::EnterDebate(const std::string& debateId, const int& user_id, DebateWrapper& debateWrapper) {
    Log::debug("[EnterDebate] User " + std::to_string(user_id) + " entering debate with id: " + debateId);

    std::vector<uint8_t> debateData = debateWrapper.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateProto.ParseFromArray(debateData.data(), static_cast<int>(debateData.size()))) {
        Log::error("[EnterDebate][ERR] Failed to parse debate protobuf for debate ID " + debateId);
        return false;
    }
    std::string rootClaimId = debateProto.root_claim_id();

    debateWrapper.moveUserToClaim(user_id, rootClaimId);

    // change some info in the user protobuf
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->set_adding_child_claim(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_sentence(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_description(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_reporting_claim(false);

    debateWrapper.updateUserProtobuf(user_id, userProto);

    return true;
}

bool MoveUserHandler::GoHome(const int& user_id, DebateWrapper& debateWrapper) {
    Log::debug("[GoHome] User " + std::to_string(user_id) + " going home");
    
    try { 
        // Get current user protobuf
        user::User userProto = debateWrapper.getUserProtobuf(user_id);
        
        userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
        // set home info
        userProto.mutable_engagement()->mutable_home_info();
        // userProto.set_debate_topic_id("");
        
        debateWrapper.updateUserProtobuf(user_id, userProto);
        
        return true;
    } catch (const std::exception& e) {
        Log::error("[GoHome][ERR] Exception: " + std::string(e.what()));
        return false;
    }
}

void MoveUserHandler::GoToClaim(const std::string& claim_id, const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    // update the current claim id to claim_id
    // save back to database;
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(claim_id);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void MoveUserHandler::GoToParentClaim(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();
    // find parent claim
    debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
    // use go to claim handler to go to parent claim
    GoToClaim(parentClaim.id(), user_id, debateWrapper);
}
