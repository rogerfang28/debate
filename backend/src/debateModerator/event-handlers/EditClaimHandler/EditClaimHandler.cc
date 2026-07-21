#include "EditClaimHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/Log.h"

void EditClaimHandler::CancelEditClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set editing_claim to false
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo::CurrentDebateAction::VIEWING_CLAIM);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void EditClaimHandler::CancelEditClaimDescription(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo::CurrentDebateAction::VIEWING_CLAIM);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void EditClaimHandler::StartEditClaim(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo::CurrentDebateAction::EDITING_CLAIM);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void EditClaimHandler::StartEditClaimDescription(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo::CurrentDebateAction::EDITING_CLAIM_DESCRIPTION);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void EditClaimHandler::SubmitEditClaim(const int& user_id, const std::string& newClaim, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();

    // Authorization: user can only edit their own claim
    debate::Claim claim = debateWrapper.getClaimById(currentClaimId);
    if (claim.creator_id() != user_id) {
        Log::warn("[SubmitEditClaim] Unauthorized edit — user " + std::to_string(user_id) + " tried to edit claim " + std::to_string(currentClaimId));
        CancelEditClaim(user_id, debateWrapper);
        return;
    }

    debateWrapper.editClaimText(currentClaimId, user_id, newClaim);
    Log::debug("[SubmitEditClaim] Closing edit claim for user: " + std::to_string(user_id));
    CancelEditClaim(user_id, debateWrapper);
}

void EditClaimHandler::SubmitEditClaimDescription(const int& user_id, const std::string& newDescription, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();

    // Authorization: user can only edit their own claim description
    debate::Claim claim = debateWrapper.getClaimById(currentClaimId);
    if (claim.creator_id() != user_id) {
        Log::warn("[SubmitEditClaimDescription] Unauthorized edit — user " + std::to_string(user_id) + " tried to modify description of claim " + std::to_string(currentClaimId));
        CancelEditClaimDescription(user_id, debateWrapper);
        return;
    }

    debateWrapper.changeDescriptionOfClaim(currentClaimId, user_id, newDescription);
    Log::debug("[SubmitEditClaimDescription] Closing edit description for user: " + std::to_string(user_id));
    CancelEditClaimDescription(user_id, debateWrapper);
}
