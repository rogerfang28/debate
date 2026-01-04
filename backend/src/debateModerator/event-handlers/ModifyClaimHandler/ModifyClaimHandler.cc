#include "ModifyClaimHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void ModifyClaimHandler::StartModifyClaim(
    DebateWrapper& debateWrapper,
    int user_id
) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    // update the user protobuf to indicate they are modifying a claim
    userProto.mutable_engagement()->mutable_debating_info()->set_modifying_current_claim(true);
    debateWrapper.updateUserProtobuf(user_id, userProto);
    // make a history save of the current claim state
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();
    debateWrapper.SaveVersionOfClaim(currentClaimId);
}

void ModifyClaimHandler::CancelModifyClaim(
    DebateWrapper& debateWrapper,
    int user_id
) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    // update the user protobuf to indicate they are no longer modifying a claim
    userProto.mutable_engagement()->mutable_debating_info()->set_modifying_current_claim(false);
    debateWrapper.updateUserProtobuf(user_id, userProto);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();
    debateWrapper.RestorePreviousVersionOfClaim(currentClaimId);
}

void ModifyClaimHandler::SubmitModifyClaim(
    DebateWrapper& debateWrapper,
    int user_id
) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    // update the user protobuf to indicate they are no longer modifying a claim
    userProto.mutable_engagement()->mutable_debating_info()->set_modifying_current_claim(false);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}