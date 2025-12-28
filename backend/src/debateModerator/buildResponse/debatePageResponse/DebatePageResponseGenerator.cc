#include "DebatePageResponseGenerator.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../utils/Log.h"
#include <iostream>
#include <vector>

void DebatePageResponseGenerator::BuildDebatePageResponse(
    moderator_to_vr::ModeratorToVRMessage& responseMessage,
    const std::string& user,
    const user::User& userProto,
    DebateWrapper& debateWrapper
) {

    user_engagement::DebatingInfo debatingInfo = userProto.engagement().debating_info();
    Log::debug("[DebatePageResponseGenerator] Building debate page response for user: " + user);
    // based on the current claim id, generate the children claims and sentences and parent claim
    std::string currentClaimId = debatingInfo.current_claim().id();
    debate::Claim currentClaim = debateWrapper.findClaim(currentClaimId);
    debatingInfo.mutable_current_claim()->set_sentence(currentClaim.sentence());
    debate::Claim parentClaim = debateWrapper.findClaim(currentClaim.parent_id());
    debatingInfo.mutable_parent_claim()->set_sentence(parentClaim.sentence());
    debatingInfo.set_current_claim_description(currentClaim.description());
    debatingInfo.mutable_parent_claim()->set_id(parentClaim.id());
    for (int i = 0; i < currentClaim.proof().claim_ids_size(); i++) {
        std::string childId = currentClaim.proof().claim_ids(i);
        debate::Claim childClaim = debateWrapper.findClaim(childId);
        user_engagement::ClaimInfo* childClaimInfo = debatingInfo.add_children_claims();
        childClaimInfo->set_id(childClaim.id());
        childClaimInfo->set_sentence(childClaim.sentence());
    }

    // find  the links under current claim proof
    std::vector<int> link_ids = debateWrapper.findLinksUnder(currentClaimId);
    for (int link_id : link_ids) {
        Log::debug("[DebatePageResponseGenerator] Processing link ID: " + std::to_string(link_id));
        debate::Link link = debateWrapper.getLinkById(link_id);
        user_engagement::LinkInfo* linkInfo = debatingInfo.add_links();
        linkInfo->set_id(std::to_string(link_id));
        linkInfo->set_connect_from(link.connect_from());
        linkInfo->set_connect_to(link.connect_to());
        linkInfo->set_connection(link.connection());
        Log::debug("[DebatePageResponseGenerator] Found link from " + link.connect_from()
            + " to " + link.connect_to()
            + " with connection: " + link.connection());
    }
    *responseMessage.mutable_engagement()->mutable_debating_info() = debatingInfo;
}
