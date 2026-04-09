#include "ConnectClaimsHandler.h"
#include "../../../utils/Log.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void ConnectClaimsHandler::ConnectClaims(
    const int& user_id,
    // const std::string& fromClaimId,
    // const std::string& toClaimId,
    const std::string& connection,
    DebateWrapper& debateWrapper
) {
    // find the claims from user engagement
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int fromClaimId = userProto.engagement().debating_info().connecting_info().from_claim_id();
    int toClaimId = userProto.engagement().debating_info().connecting_info().to_claim_id();
    int debateId = debateWrapper.findDebateId(fromClaimId);
    // std::string& connection = userProto.engagement().debating_info().connecting_info().connection();
    // this one should actually update the links database
    int linkId = debateWrapper.addLink(fromClaimId, toClaimId, connection, user_id, debateId);
    CancelConnectClaims(user_id, debateWrapper);
    
}

void ConnectClaimsHandler::ConnectFromClaim(
    const int& user_id,
    int fromClaimId,
    DebateWrapper& debateWrapper
) {
    // take user proto and put connect from claim);
    user_engagement::UserEngagement userEngagement;
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CONNECTING_CLAIMS);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_from_claim_id(fromClaimId);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_connecting(true);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ConnectClaimsHandler::ConnectToClaim(
    const int& user_id,
    int toClaimId,
    DebateWrapper& debateWrapper
) {
    // take user proto and put connect to claim
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_to_claim_id(toClaimId);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_opened_connect_modal(true);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ConnectClaimsHandler::CancelConnectClaims(
    const int& user_id,
    DebateWrapper& debateWrapper
) {
    // take user proto and cancel connect claims
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_connecting(false);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->set_opened_connect_modal(false);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->clear_from_claim_id();
    userProto.mutable_engagement()->mutable_debating_info()->mutable_connecting_info()->clear_to_claim_id();
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo_CurrentDebateAction_ActionType_VIEWING_CLAIM);
    debateWrapper.updateUserProtobuf(user_id, userProto);

}

void ConnectClaimsHandler::DeleteLinkById(
    const int& user_id,
    int linkId,
    DebateWrapper& debateWrapper
) {
    // first find the link
    debate::Link linkProto = debateWrapper.getLinkById(linkId);
    int fromClaimId = linkProto.connect_from();
    int toClaimId = linkProto.connect_to();

    // remove link id from both the parent claim and top-level link list
    debate::Claim parentClaim = debateWrapper.findClaimParent(fromClaimId);
    auto removeLinkId = [linkId](debate::Claim& claim) {
        auto* proof = claim.mutable_proof();
        for (int i = 0; i < proof->link_ids_size(); ++i) {
            if (proof->link_ids(i) == linkId) {
                proof->mutable_link_ids()->SwapElements(i, proof->link_ids_size() - 1);
                proof->mutable_link_ids()->RemoveLast();
                break;
            }
        }

        auto& linkIds = *claim.mutable_link_ids();
        for (int i = 0; i < linkIds.size(); ++i) {
            if (linkIds[i] == linkId) {
                linkIds.erase(linkIds.begin() + i);
                break;
            }
        }
    };

    debate::Claim fromClaim = debateWrapper.getClaimById(fromClaimId);
    removeLinkId(fromClaim);
    debateWrapper.updateClaimInDB(fromClaim);

    if (toClaimId != fromClaimId) {
        debate::Claim toClaim = debateWrapper.getClaimById(toClaimId);
        removeLinkId(toClaim);
        debateWrapper.updateClaimInDB(toClaim);
    }

    if (parentClaim.id() != fromClaim.id()) {
        // keep the legacy parent lookup path in sync too
        removeLinkId(parentClaim);
        debateWrapper.updateClaimInDB(parentClaim);
    }
    // then delete the link from database
    // debateWrapper.deleteLinkById(linkId);
}