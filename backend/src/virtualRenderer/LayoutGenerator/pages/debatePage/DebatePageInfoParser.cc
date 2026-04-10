#include "DebatePageInfoParser.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include "../../../../../src/gen/cpp/collection.pb.h"
#include "../../../../utils/Log.h"
#include <unordered_set>

namespace {
rendering_info::ScopeType MapScopeType(debate::ScopeType scopeType) {
	switch (scopeType) {
		case debate::SINGLE_CLAIM:
			return rendering_info::SINGLE_CLAIM;
		case debate::FULL_DEBATE:
			return rendering_info::FULL_DEBATE;
		default:
			return rendering_info::SCOPE_TYPE_UNSPECIFIED;
	}
}

rendering_info::ClaimStatus MapClaimStatus(debate::ClaimStatus status) {
	switch (status) {
		case debate::ClaimStatus::NEUTRAL:
			return rendering_info::CLAIM_STATUS_NEUTRAL;
		case debate::ClaimStatus::CHALLENGED:
			return rendering_info::CLAIM_STATUS_CHALLENGED;
		case debate::ClaimStatus::DEFENDED:
			return rendering_info::CLAIM_STATUS_DEFENDED;
		case debate::ClaimStatus::DISPROVEN:
			return rendering_info::CLAIM_STATUS_DISPROVEN;
		default:
			return rendering_info::CLAIM_STATUS_UNSPECIFIED;
	}
}

rendering_info::ChallengeStatus MapChallengeStatus(debate::ChallengeStatus status) {
	switch (status) {
		case debate::ChallengeStatus::ONGOING:
			return rendering_info::CHALLENGE_STATUS_ONGOING;
		case debate::ChallengeStatus::CONCEDED:
			return rendering_info::CHALLENGE_STATUS_CONCEDED;
		case debate::ChallengeStatus::PROVEN:
			return rendering_info::CHALLENGE_STATUS_PROVEN;
		default:
			return rendering_info::CHALLENGE_STATUS_UNSPECIFIED;
	}
}

rendering_info::DebateActionType MapDebateAction(
	user_engagement::DebatingInfo_CurrentDebateAction_ActionType actionType
) {
	switch (actionType) {
		case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_VIEWING_CLAIM:
			return rendering_info::VIEWING_CLAIM;
		case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_ADDING_CHILD_CLAIM:
			return rendering_info::ADDING_CHILD_CLAIM;
		case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CONNECTING_CLAIMS:
			return rendering_info::CONNECTING_CLAIMS;
		case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CHALLENGING_CLAIM:
			return rendering_info::CHALLENGING_CLAIM;
		case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_EDITING_CLAIM_DESCRIPTION:
			return rendering_info::EDITING_CLAIM_DESCRIPTION;
		case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_EDITING_CLAIM:
			return rendering_info::EDITING_CLAIM;
		case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_REPORTING_CLAIM:
			return rendering_info::REPORTING_CLAIM;
		default:
			return rendering_info::ACTION_TYPE_UNSPECIFIED;
	}
}
}

rendering_info::DebatePageRenderingInfo DebatePageInfoParser::ParseFromUser(const user::User& userProto, const debate::Collection& collectionProto) {
	rendering_info::DebatePageRenderingInfo info;
	Log::info("[DebatePageInfoParser] collection received: claims_by_id=" + std::to_string(collectionProto.claims_by_id_size()) + ", links_by_id=" + std::to_string(collectionProto.links_by_id_size()));

	
	info.set_viewer_user_id(userProto.user_id());
	info.set_viewer_username(userProto.username());
	info.set_scope_type(MapScopeType(userProto.current_scope().scopetype()));

	const user_engagement::DebatingInfo& debatingInfo = userProto.engagement().debating_info();

	info.set_debate_id(debatingInfo.debate_id());
	info.set_current_claim_description(debatingInfo.current_claim_description());
	info.set_modifying_current_claim(debatingInfo.modifying_current_claim());
	info.set_current_action(MapDebateAction(debatingInfo.current_debate_action().action_type()));

	rendering_info::ClaimRenderInfo* currentClaim = info.mutable_current_claim();
	currentClaim->set_id(debatingInfo.current_claim().id());
	currentClaim->set_sentence(debatingInfo.current_claim().sentence());
	currentClaim->set_creator_id(debatingInfo.current_claim().creator_id());
	currentClaim->set_status(MapClaimStatus(debatingInfo.current_claim().status()));
	info.set_is_challenge_debate(false);

	std::unordered_set<int> visibleClaimIds;
	visibleClaimIds.insert(currentClaim->id());

	int currentClaimId = debatingInfo.current_claim().id();
	Log::debug("[DebatePageInfoParser] Looking up current_claim_id=" + std::to_string(currentClaimId) + " in collection.claims_by_id (size=" + std::to_string(collectionProto.claims_by_id_size()) + ")");
	
	auto currentClaimIt = collectionProto.claims_by_id().find(currentClaimId);
	if (currentClaimIt == collectionProto.claims_by_id().end()) {
		Log::warn("[DebatePageInfoParser] Current claim ID " + std::to_string(currentClaimId) + " NOT FOUND in collection!");
		Log::info("[DebatePageInfoParser] Finished copying links: output_count=" + std::to_string(info.links_size()));
		return info;
	}

	debate::Claim currentClaimProto = currentClaimIt->second;
	Log::info("[DebatePageInfoParser] Current claim ID: " + std::to_string(currentClaimProto.id()) + ", has " + std::to_string(currentClaimProto.link_ids_size()) + " links");
	if (currentClaimProto.link_ids_size() == 0) {
		Log::warn("[DebatePageInfoParser] Current claim has NO link_ids - no parent-child relationships defined!");
	}
	for (int i = 0; i < currentClaimProto.link_ids_size(); ++i) {
		int linkId = currentClaimProto.link_ids(i);
		Log::debug("[DebatePageInfoParser] Processing current claim link_id=" + std::to_string(linkId));
		auto linkIt = collectionProto.links_by_id().find(linkId);
		if (linkIt == collectionProto.links_by_id().end()) {
			Log::warn("[DebatePageInfoParser] Link ID " + std::to_string(linkId) + " not found in collection!");
			continue;
		}
		debate::Link linkProto = linkIt->second;
		Log::debug("[DebatePageInfoParser] link snapshot from=" + std::to_string(linkProto.connect_from()) + ", to=" + std::to_string(linkProto.connect_to()) + ", type=" + std::to_string(linkProto.link_type()) + ", connection=\"" + linkProto.connection() + "\"");
		if (linkProto.connect_from() == currentClaimProto.id() && linkProto.link_type() == debate::LinkType::CHALLENGE) {
			info.set_is_challenge_debate(true);
			Log::debug("[DebatePageInfoParser] Current claim is part of a challenge debate because it has an outgoing CHALLENGE link to claim_id=" + std::to_string(linkProto.connect_to()));
		}
		if (linkProto.connect_from() == currentClaimProto.id() && linkProto.link_type() == debate::LinkType::PARENT_CHILD) {
			int childClaimId = linkProto.connect_to();
			Log::debug("[DebatePageInfoParser] Found parent-child link, child_claim_id=" + std::to_string(childClaimId));
			auto childIt = collectionProto.claims_by_id().find(childClaimId);
			if (childIt != collectionProto.claims_by_id().end()) {
				const debate::Claim& childClaim = childIt->second;
				Log::debug("[DebatePageInfoParser] Adding child claim id=" + std::to_string(childClaim.id()) + ", sentence=\"" + childClaim.sentence() + "\", creator_id=" + std::to_string(childClaim.creator_id()) + ", status=" + std::to_string(childClaim.status()));
				rendering_info::ClaimRenderInfo* outChild = info.add_children_claims();
				outChild->set_id(childClaim.id());
				outChild->set_sentence(childClaim.sentence());
				outChild->set_creator_id(childClaim.creator_id());
				outChild->set_status(MapClaimStatus(childClaim.status()));
				visibleClaimIds.insert(childClaim.id());
			} else {
				Log::warn("[DebatePageInfoParser] Child claim " + std::to_string(childClaimId) + " not found in collection");
			}
		}
	}
	Log::debug("[DebatePageInfoParser] children_claims copied=" + std::to_string(info.children_claims_size()));
	for (int i = 0; i < info.children_claims_size(); ++i) {
		const rendering_info::ClaimRenderInfo& child = info.children_claims(i);
		Log::debug("[DebatePageInfoParser] output child[" + std::to_string(i) + "] id=" + std::to_string(child.id()) + ", sentence=\"" + child.sentence() + "\", creator_id=" + std::to_string(child.creator_id()) + ", status=" + std::to_string(child.status()));
	}

	Log::info(
		"[DebatePageInfoParser] Copying collection links into rendering_info.links: links_by_id_count=" +
		std::to_string(collectionProto.links_by_id_size()) +
		", visible_claim_count=" + std::to_string(visibleClaimIds.size())
	);
	int outputLinkIndex = 0;
	for (const auto& entry : collectionProto.links_by_id()) {
		const int linkId = entry.first;
		const debate::Link& link = entry.second;
		const bool fromVisible = visibleClaimIds.count(link.connect_from()) > 0;
		const bool toVisible = visibleClaimIds.count(link.connect_to()) > 0;
		const bool isParentChild = (link.link_type() == debate::LinkType::PARENT_CHILD);

		if (!fromVisible || !toVisible || isParentChild) {
			continue;
		}

		Log::debug(
			"[DebatePageInfoParser] Source collection link id=" + std::to_string(linkId) +
			", from=" + std::to_string(link.connect_from()) +
			", to=" + std::to_string(link.connect_to()) +
			", creator_id=" + std::to_string(link.creator_id()) +
			", connection=\"" + link.connection() + "\""
		);
		rendering_info::LinkRenderInfo* outLink = info.add_links();
		outLink->set_id(linkId);
		outLink->set_connect_from(link.connect_from());
		outLink->set_connect_to(link.connect_to());
		outLink->set_connection(link.connection());
		outLink->set_creator_id(link.creator_id());
		Log::debug(
			"[DebatePageInfoParser] Output link[" + std::to_string(outputLinkIndex) + "] id=" + std::to_string(outLink->id()) +
			", from=" + std::to_string(outLink->connect_from()) +
			", to=" + std::to_string(outLink->connect_to()) +
			", creator_id=" + std::to_string(outLink->creator_id()) +
			", connection=\"" + outLink->connection() + "\""
		);
		++outputLinkIndex;
	}
	Log::info(
		"[DebatePageInfoParser] Finished copying links: output_count=" + std::to_string(info.links_size())
	);

	// Find challenge links pointing to current claim from collection
	Log::test("[DebatePageInfoParser] Scanning for CHALLENGE links pointing to current_claim_id=" + std::to_string(currentClaimId) + ", total_links_in_collection=" + std::to_string(collectionProto.links_by_id_size()));
	int challengeLinkCount = 0;
	for (const auto& entry : collectionProto.links_by_id()) {
		challengeLinkCount++;
		const debate::Link& link = entry.second;
		Log::test("[DebatePageInfoParser] Checking link #" + std::to_string(challengeLinkCount) + ": id=" + std::to_string(entry.first) + ", link_type=" + std::to_string(link.link_type()) + ", from=" + std::to_string(link.connect_from()) + ", to=" + std::to_string(link.connect_to()));
		
		// Look for challenge links pointing to current claim
		if (link.link_type() == debate::LinkType::CHALLENGE && 
			link.connect_to() == currentClaimId) {
			
			Log::test("[DebatePageInfoParser] Found CHALLENGE link pointing to current claim! link_id=" + std::to_string(entry.first) + ", from_claim_id=" + std::to_string(link.connect_from()));
			int challengeClaimId = link.connect_from();
			
			// Find the challenge claim in collection
			auto challengeClaimIt = collectionProto.claims_by_id().find(challengeClaimId);
			if (challengeClaimIt != collectionProto.claims_by_id().end()) {
				const debate::Claim& challengeClaim = challengeClaimIt->second;
				
				rendering_info::ChallengeRenderInfo* outChallenge = info.add_current_challenges();
				outChallenge->set_id(challengeClaimId);
				outChallenge->set_sentence(challengeClaim.sentence());
				outChallenge->set_creator_id(challengeClaim.creator_id());
				outChallenge->set_status(rendering_info::CHALLENGE_STATUS_ONGOING);
				Log::test("[DebatePageInfoParser] Added CHALLENGE to rendering_info: id=" + std::to_string(challengeClaimId) + ", sentence=\"" + challengeClaim.sentence() + "\", creator_id=" + std::to_string(challengeClaim.creator_id()));
			} else {
				Log::warn("[DebatePageInfoParser] Challenge claim id=" + std::to_string(challengeClaimId) + " NOT FOUND in collection!");
			}
		}
	}
	Log::test("[DebatePageInfoParser] Finished scanning challenges: looped_" + std::to_string(challengeLinkCount) + "_times, total_challenges_added=" + std::to_string(info.current_challenges_size()));

	if (debatingInfo.has_connecting_info()) {
		const user_engagement::DebatingInfo_ConnectingInfo& srcConnecting = debatingInfo.connecting_info();
		Log::info(
			"[DebatePageInfoParser] connecting_info present: from_claim_id=" + std::to_string(srcConnecting.from_claim_id()) +
			", to_claim_id=" + std::to_string(srcConnecting.to_claim_id()) +
			", connecting=" + std::string(srcConnecting.connecting() ? "true" : "false") +
			", opened_connect_modal=" + std::string(srcConnecting.opened_connect_modal() ? "true" : "false")
		);
		rendering_info::ConnectingRenderInfo* dstConnecting = info.mutable_connecting_info();
		dstConnecting->set_from_claim_id(srcConnecting.from_claim_id());
		dstConnecting->set_to_claim_id(srcConnecting.to_claim_id());
		dstConnecting->set_connecting(srcConnecting.connecting());
		dstConnecting->set_opened_connect_modal(srcConnecting.opened_connect_modal());
		Log::info(
			"[DebatePageInfoParser] connecting_info copied: from_claim_id=" + std::to_string(dstConnecting->from_claim_id()) +
			", to_claim_id=" + std::to_string(dstConnecting->to_claim_id()) +
			", connecting=" + std::string(dstConnecting->connecting() ? "true" : "false") +
			", opened_connect_modal=" + std::string(dstConnecting->opened_connect_modal() ? "true" : "false")
		);
	} else {
		Log::info("[DebatePageInfoParser] connecting_info missing");
	}

	if (debatingInfo.has_challenging_info()) {
		const user_engagement::DebatingInfo_ChallengingInfo& srcChallenging = debatingInfo.challenging_info();
		rendering_info::ChallengingRenderInfo* dstChallenging = info.mutable_challenging_info();
		for (int i = 0; i < srcChallenging.claim_ids_size(); ++i) {
			dstChallenging->add_claim_ids(srcChallenging.claim_ids(i));
		}
		for (int i = 0; i < srcChallenging.link_ids_size(); ++i) {
			dstChallenging->add_link_ids(srcChallenging.link_ids(i));
		}
		dstChallenging->set_opened_challenge_modal(srcChallenging.opened_challenge_modal());
	}

	return info;
}
