#include "EditClaimHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/Log.h"

void EditClaimHandler::CancelEditClaim(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set editing_claim to false
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_sentence(false);

    debateWrapper.updateUserProtobuf(user, userProto);
}

void EditClaimHandler::CancelEditClaimDescription(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set editing_claim to false
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_description(false);

    debateWrapper.updateUserProtobuf(user, userProto);
}

void EditClaimHandler::StartEditClaim(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set editing_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_sentence(true);

    debateWrapper.updateUserProtobuf(user, userProto);
}


void EditClaimHandler::StartEditClaimDescription(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set editing_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_description(true);
    debateWrapper.updateUserProtobuf(user, userProto);
}

void EditClaimHandler::SubmitEditClaim(const std::string& user, const std::string& newClaim, DebateWrapper& debateWrapper) {
    // update the claim text in the debate wrapper
    // find current claim
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();

    // change the claim text
    debateWrapper.editClaimText(currentClaimId, newClaim);

    // close
    Log::debug("[SubmitEditClaim] Closing edit claim for user: " + user);
    CancelEditClaim(user, debateWrapper);
}

void EditClaimHandler::SubmitEditClaimDescription(const std::string& user, const std::string& newDescription, DebateWrapper& debateWrapper) {
    // update the claim description in the debate wrapper
    // find current claim
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();

    // change the description
    debateWrapper.changeDescriptionOfClaim(
        currentClaimId,
        newDescription);

    // close
    Log::debug("[SubmitEditClaimDescription] Closing edit description for user: " + user);
    CancelEditClaimDescription(user, debateWrapper);
}