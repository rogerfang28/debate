#include "DebatePageResponseGenerator.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../utils/Log.h"
#include <iostream>
#include <vector>
#include "./BuildCollection.h"

void DebatePageResponseGenerator::BuildDebatePageResponse(
    moderator_to_vr::ModeratorToVRMessage& responseMessage,
    const int& user_id,
    const user::User& userProto,
    DebateWrapper& debateWrapper
) {
    user_engagement::DebatingInfo debatingInfo = userProto.engagement().debating_info();
    Log::info("[DebatePageResponseGenerator] Building debate page response for user: " + std::to_string(user_id));
    // based on the current claim id, generate the children claims and sentences and parent claim
    int currentClaimId = debatingInfo.current_claim().id();
    debate::Claim currentClaim = debateWrapper.getClaimById(currentClaimId);
    debatingInfo.mutable_current_claim()->set_sentence(currentClaim.sentence());
    debatingInfo.mutable_current_claim()->set_id(currentClaim.id()); // already there
    debatingInfo.mutable_current_claim()->set_creator_id(currentClaim.creator_id());

    debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
    debatingInfo.mutable_parent_claim()->set_sentence(parentClaim.sentence());
    debatingInfo.set_current_claim_description(currentClaim.description());
    debatingInfo.mutable_parent_claim()->set_id(parentClaim.id());
    debatingInfo.mutable_parent_claim()->set_creator_id(parentClaim.creator_id());

    // find  the links under current claim proof
    std::vector<int> link_ids = debateWrapper.findLinksUnder(currentClaimId);
    for (int link_id : link_ids) {
        Log::debug("[DebatePageResponseGenerator] Processing link ID: " + std::to_string(link_id));
        debate::Relationship::Link link = debateWrapper.getLinkById(link_id).link();
        user_engagement::LinkInfo* linkInfo = debatingInfo.add_links();
        linkInfo->set_id(link_id);
        linkInfo->set_connect_from(link.connect_from());
        linkInfo->set_connect_to(link.connect_to());
        linkInfo->set_connection(link.connection());
        linkInfo->set_creator_id(link.creator_id());
        Log::debug("[DebatePageResponseGenerator] Found link from " + std::to_string(link.connect_from())
            + " to " + std::to_string(link.connect_to())
            + " with connection: " + link.connection());
    }
    // set the statement collection
    debate::Collection* collection = responseMessage.mutable_collection();
    // find all the users in the debate
    std::vector<int> user_ids = debateWrapper.findUsersInDebate(debatingInfo.debate_id());
    debate::CollectionSpecification collectionSpec = userProto.collection_spec();
    *collection = BuildCollection::BuildForDebateAndUsers(collectionSpec.debate_id(), user_ids, debateWrapper);
    // user_ids for now is still all users

    *responseMessage.mutable_user()->mutable_engagement()->mutable_debating_info() = debatingInfo;
}
