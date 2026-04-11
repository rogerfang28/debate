#include "MoveUserHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include "../../../utils/Log.h"
#include "../../../utils/pathUtils.h"
#include <iostream>
#include <vector>

bool MoveUserHandler::EnterDebate(const int& debateId, const int& user_id, DebateWrapper& debateWrapper) {
    resetOngoingActivities(user_id, debateWrapper);
    Log::debug("[EnterDebate] User " + std::to_string(user_id) + " entering debate with id: " + std::to_string(debateId));

    std::vector<uint8_t> debateData = debateWrapper.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateProto.ParseFromArray(debateData.data(), static_cast<int>(debateData.size()))) {
        Log::error("[EnterDebate][ERR] Failed to parse debate protobuf for debate ID " + std::to_string(debateId));
        return false;
    }
    int rootClaimId = debateProto.root_claim_id();

    debateWrapper.moveUserToClaim(user_id, rootClaimId);

    // change some info in the user protobuf
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->set_is_challenge(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_debate_id(debateId);
    userProto.mutable_engagement()->mutable_debating_info()->set_challenged_user_id(0);
    userProto.mutable_engagement()->mutable_debating_info()->set_challenger_user_id(0);
    userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_DEBATING);
    userProto.mutable_current_scope()->set_scopetype(debate::SINGLE_CLAIM);
    userProto.mutable_current_scope()->mutable_full_debate()->set_debate_id(debateId);
    resetOngoingActivities(user_id, debateWrapper);

    // change the collection spec to be the current debate
    userProto.mutable_collection_spec()->set_debate_id(debateId);
    debateWrapper.updateUserProtobuf(user_id, userProto);

    return true;
}

bool MoveUserHandler::GoHome(const int& user_id, DebateWrapper& debateWrapper) {
    resetOngoingActivities(user_id, debateWrapper);
    Log::debug("[GoHome] User " + std::to_string(user_id) + " going home");
    
    try { 
        // Get current user protobuf
        user::User userProto = debateWrapper.getUserProtobuf(user_id);
        
        userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
        // set home info
        userProto.mutable_engagement()->mutable_home_info();
        
        // fill info for home page engagement
        // user_engagement::DebateList debate_list = debateWrapper.FillUserDebateList(user_id);

        debateWrapper.updateUserProtobuf(user_id, userProto);
        
        return true;
    } catch (const std::exception& e) {
        Log::error("[GoHome][ERR] Exception: " + std::string(e.what()));
        return false;
    }
}

void MoveUserHandler::GoToClaim(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper) {
    resetOngoingActivities(user_id, debateWrapper);
    Log::debug("[GoToClaim] User " + std::to_string(user_id) + " going to claim with id: " + std::to_string(claim_id));
    // update information based on the claim
    // get the user from the database
    // update the current claim id to claim_id
    // save back to database;
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(claim_id);
    // change scope
    userProto.mutable_current_scope()->mutable_single_claim()->set_current_claim_id(claim_id);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

// this function should be not called later, since vr can just call go to claim with claim id
void MoveUserHandler::GoToParentClaim(const int& user_id, DebateWrapper& debateWrapper) {
    resetOngoingActivities(user_id, debateWrapper);
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();
    // find parent claim
    debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
    // use go to claim handler to go to parent claim
    GoToClaim(parentClaim.id(), user_id, debateWrapper);
}

void MoveUserHandler::GoToChallenge(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper) {
    Log::debug("[GoToChallenge] User " + std::to_string(user_id) + " going to claim id: " + std::to_string(claim_id));
    GoToClaim(claim_id, user_id, debateWrapper);
}

void MoveUserHandler::GoToParentClaimOfDebate(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();

    Log::debug("[GoToParentClaimOfDebate] User " + std::to_string(user_id) + " going to parent claim from current claim id: " + std::to_string(currentClaimId));

    debate::Claim currentClaim = debateWrapper.getClaimById(currentClaimId);
    int challengedClaimId = -1;
    for (int i = 0; i < currentClaim.link_ids_size(); ++i) {
        int linkId = currentClaim.link_ids(i);
        debate::Link linkProto = debateWrapper.getLinkById(linkId);
        if (linkProto.link_type() == debate::LinkType::CHALLENGE && linkProto.connect_from() == currentClaimId) {
            challengedClaimId = linkProto.connect_to();
            Log::debug("[GoToParentClaimOfDebate] Found CHALLENGE link id: " + std::to_string(linkId) + " from claim " + std::to_string(currentClaimId) + " to challenged claim " + std::to_string(challengedClaimId));
            break;
        }
    }

    if (challengedClaimId != -1) {
        GoToClaim(challengedClaimId, user_id, debateWrapper);
        Log::debug("[GoToParentClaimOfDebate] Now going to challenged claim id: " + std::to_string(challengedClaimId));
        resetOngoingActivities(user_id, debateWrapper);
        return;
    }
    Log::warn("[GoToParentClaimOfDebate] No outgoing CHALLENGE link found from current claim id: " + std::to_string(currentClaimId));
    resetOngoingActivities(user_id, debateWrapper);
}

void MoveUserHandler::resetOngoingActivities(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    // reset connecting info
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_connecting(false);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_from_claim_id(0);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_to_claim_id(0);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_opened_connect_modal(false);
    
    // reset challenging info
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo::CurrentDebateAction::VIEWING_CLAIM);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->clear_claim_ids();
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->clear_link_ids();
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->set_challenge_sentence("");
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->set_opened_challenge_modal(false);
    
    debateWrapper.updateUserProtobuf(user_id, userProto);
}