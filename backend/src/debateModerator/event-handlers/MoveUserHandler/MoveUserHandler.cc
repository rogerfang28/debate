#include "MoveUserHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include "../../../utils/Log.h"
#include "../../../utils/pathUtils.h"
#include <iostream>
#include <vector>

bool MoveUserHandler::EnterDebate(const std::string& debateId, const std::string& user, DebateWrapper& debateWrapper) {
    Log::debug("[EnterDebate] User " + user + " entering debate with id: " + debateId);

    std::vector<uint8_t> debateData = debateWrapper.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateProto.ParseFromArray(debateData.data(), static_cast<int>(debateData.size()))) {
        Log::error("[EnterDebate][ERR] Failed to parse debate protobuf for debate ID " + debateId);
        return false;
    }
    std::string rootClaimId = debateProto.root_claim_id();

    debateWrapper.moveUserToClaim(user, rootClaimId);

    // change some info in the user protobuf
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    userProto.mutable_engagement()->mutable_debating_info()->set_adding_child_claim(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_sentence(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_description(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_reporting_claim(false);

    debateWrapper.updateUserProtobuf(user, userProto);

    return true;
}

bool MoveUserHandler::GoHome(const std::string& user, DebateWrapper& debateWrapper) {
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
        
        debateWrapper.updateUserProtobuf(user, userProto);
        
        return true;
    } catch (const std::exception& e) {
        Log::error("[GoHome][ERR] Exception: " + std::string(e.what()));
        return false;
    }
}

void MoveUserHandler::GoToClaim(const std::string& claim_id, const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    // update the current claim id to claim_id
    // save back to database;
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(claim_id);
    debateWrapper.updateUserProtobuf(user, userProto);
}

void MoveUserHandler::GoToParentClaim(const std::string& user, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();
    // find parent claim
    debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
    // use go to claim handler to go to parent claim
    GoToClaim(parentClaim.id(), user, debateWrapper);
}
