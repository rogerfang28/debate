#include "EditClaimHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/Log.h"

void EditClaimHandler::CancelEditClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set editing_claim to false
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_sentence(false);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void EditClaimHandler::CancelEditClaimDescription(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set editing_claim to false
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_description(false);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void EditClaimHandler::StartEditClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set editing_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_sentence(true);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}


void EditClaimHandler::StartEditClaimDescription(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set editing_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_description(true);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void EditClaimHandler::SubmitEditClaim(const int& user_id, const std::string& newClaim, DebateWrapper& debateWrapper) {
    // update the claim text in the debate wrapper
    // find current claim
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();

    // change the claim text
    debateWrapper.editClaimText(currentClaimId, newClaim);

    // close
    Log::debug("[SubmitEditClaim] Closing edit claim for user: " + std::to_string(user_id));
    CancelEditClaim(user_id, debateWrapper);
}

void EditClaimHandler::SubmitEditClaimDescription(const int& user_id, const std::string& newDescription, DebateWrapper& debateWrapper) {
    // update the claim description in the debate wrapper
    // find current claim
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();

    // change the description
    debateWrapper.changeDescriptionOfClaim(
        currentClaimId,
        newDescription);

    // close
    Log::debug("[SubmitEditClaimDescription] Closing edit description for user: " + std::to_string(user_id));
    CancelEditClaimDescription(user_id, debateWrapper);
}