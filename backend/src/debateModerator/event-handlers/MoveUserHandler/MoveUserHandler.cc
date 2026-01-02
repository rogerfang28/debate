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
    bool isChallenge = debateProto.is_challenge();
    int rootClaimId = debateProto.root_claim_id();
    int challenger_id = debateProto.creator_id();
    int challenged_id;
    // find the challenged id, it will be the owner of the debate being challenged
    int challenged_parent_claim_id = debateProto.parent_challenge_id();
    debate::Claim challengedParentClaim = debateWrapper.getClaimById(challenged_parent_claim_id);
    challenged_id = challengedParentClaim.creator_id();

    debateWrapper.moveUserToClaim(user_id, rootClaimId);

    // change some info in the user protobuf
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->set_is_challenge(isChallenge);
    userProto.mutable_engagement()->mutable_debating_info()->set_debate_id(debateId);
    userProto.mutable_engagement()->mutable_debating_info()->set_challenged_user_id(challenged_id);
    userProto.mutable_engagement()->mutable_debating_info()->set_challenger_user_id(challenger_id);
    userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_DEBATING);
    resetOngoingActivities(user_id, debateWrapper);
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
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void MoveUserHandler::GoToParentClaim(const int& user_id, DebateWrapper& debateWrapper) {
    resetOngoingActivities(user_id, debateWrapper);
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();
    // find parent claim
    debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
    // use go to claim handler to go to parent claim
    GoToClaim(parentClaim.id(), user_id, debateWrapper);
}

void MoveUserHandler::GoToChallenge(const int& challenge_id, const int& user_id, DebateWrapper& debateWrapper) {
    // get challenge id, find the debate, enterdebate debateid
    debate::Challenge challengeProto = debateWrapper.getChallengeProtobuf(challenge_id);
    int debateId = challengeProto.proof_debate_id();
    Log::debug("[GoToChallenge] User " + std::to_string(user_id) + " going to challenge with proof debate id: " + std::to_string(debateId));
    EnterDebate(debateId, user_id, debateWrapper);
    resetOngoingActivities(user_id, debateWrapper);
}

void MoveUserHandler::GoToParentClaimOfDebate(const int& user_id, DebateWrapper& debateWrapper) {
    // find debate_id
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int debate_id = userProto.engagement().debating_info().debate_id();

    // find debate and find parent challenge id
    Log::debug("[GoToParentClaimOfDebate] User " + std::to_string(user_id) + " going to parent claim of debate id: " + std::to_string(debate_id));
    debate::Debate debateProto;
    std::vector<uint8_t> debateData = debateWrapper.getDebateProtobuf(debate_id);
    debateProto.ParseFromArray(debateData.data(), debateData.size());
    int challenge_id = debateProto.parent_challenge_id();
    
    // get challenge id, find the parent claim of the challenge, go to that claim
    debate::Challenge challengeProto = debateWrapper.getChallengeProtobuf(challenge_id);
    int parentClaim = challengeProto.challenged_parent_claim_id();
    debate::Claim claimProto = debateWrapper.getClaimById(parentClaim);
    int new_debate_id = claimProto.debate_id();
    EnterDebate(new_debate_id, user_id, debateWrapper);
    Log::debug("[GoToParentClaimOfDebate] Now going to parent claim id: " + std::to_string(parentClaim));
    GoToClaim(parentClaim, user_id, debateWrapper);
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