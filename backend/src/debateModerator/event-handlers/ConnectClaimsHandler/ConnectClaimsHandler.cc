#include "ConnectClaimsHandler.h"
#include "../../../utils/Log.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void ConnectClaimsHandler::ConnectClaims(
    const std::string& user,
    // const std::string& fromClaimId,
    // const std::string& toClaimId,
    const std::string& connection,
    DebateWrapper& debateWrapper
) {
    // find the claims from user engagement
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    std::string fromClaimId = userProto.engagement().debating_info().connecting_info().from_claim_id();
    std::string toClaimId = userProto.engagement().debating_info().connecting_info().to_claim_id();
    // std::string& connection = userProto.engagement().debating_info().connecting_info().connection();
    // this one should actually update the links database
    int linkId = debateWrapper.addLink(fromClaimId, toClaimId, connection, user);
    // also add it to the claims proof id
    debate::Claim parentClaim = debateWrapper.findClaimParent(fromClaimId);
    Log::debug("[ConnectClaimsHandler] Adding link ID " + std::to_string(linkId) + " to parent claim ID " + parentClaim.id());
    parentClaim.mutable_proof()->add_link_ids(std::to_string(linkId));
    debateWrapper.updateClaimInDB(parentClaim);
    CancelConnectClaims(user, debateWrapper);
    
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

void ConnectClaimsHandler::DeleteLinkById(
    const std::string& user,
    int linkId,
    DebateWrapper& debateWrapper
) {
    // first find the link
    debate::Link linkProto = debateWrapper.getLinkById(linkId);
    std::string fromClaimId = linkProto.connect_from();
    // remove link id from parent claim's proof
    debate::Claim parentClaim = debateWrapper.findClaimParent(fromClaimId);
    auto* proof = parentClaim.mutable_proof();
    for (int i = 0; i < proof->link_ids_size(); ++i) {
        if (proof->link_ids(i) == std::to_string(linkId)) {
            proof->mutable_link_ids()->DeleteSubrange(i, 1);
            break;
        }
    }
    debateWrapper.updateClaimInDB(parentClaim);
    // then delete the link from database
    debateWrapper.deleteLinkById(linkId);
}