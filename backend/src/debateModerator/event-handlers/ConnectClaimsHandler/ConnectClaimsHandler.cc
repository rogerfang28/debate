#include "ConnectClaimsHandler.h"
#include "../../../utils/Log.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include "../../../../../src/gen/cpp/debate.pb.h"

void ConnectClaimsHandler::ConnectClaims(
    const std::string& user,
    const std::string& fromClaimId,
    const std::string& toClaimId,
    const std::string& connection,
    DebateWrapper& debateWrapper
) {
    // this one should actually update the claim protobuf
    
}

void ConnectClaimsHandler::ConnectFromClaim(
    const std::string& user,
    const std::string& fromClaimId,
    DebateWrapper& debateWrapper
) {
    // take user proto and put connect from claim);
    user_engagement::UserEngagement userEngagement;
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_from_claim_id(fromClaimId);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_connecting(true);

    debateWrapper.updateUserProtobuf(user, userProto);
}

void ConnectClaimsHandler::ConnectToClaim(
    const std::string& user,
    const std::string& toClaimId,
    DebateWrapper& debateWrapper
) {
    // take user proto and put connect to claim
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_to_claim_id(toClaimId);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_opened_connect_modal(true);

    debateWrapper.updateUserProtobuf(user, userProto);
}

void ConnectClaimsHandler::CancelConnectClaims(
    const std::string& user,
    DebateWrapper& debateWrapper
) {
    // take user proto and cancel connect claims
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_connecting(false);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_opened_connect_modal(false);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->clear_from_claim_id();
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->clear_to_claim_id();

    debateWrapper.updateUserProtobuf(user, userProto);

}