#include "DebatePageInfoParser.h"

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

rendering_info::DebatePageRenderingInfo DebatePageInfoParser::ParseFromUser(const user::User& userProto) {
	rendering_info::DebatePageRenderingInfo info;

	info.set_viewer_user_id(userProto.user_id());
	info.set_viewer_username(userProto.username());
	info.set_scope_type(MapScopeType(userProto.current_scope().scopetype()));

	const user_engagement::DebatingInfo& debatingInfo = userProto.engagement().debating_info();

	info.set_debate_id(debatingInfo.debate_id());
	info.set_is_challenge_debate(debatingInfo.is_challenge());
	info.set_current_claim_description(debatingInfo.current_claim_description());
	info.set_modifying_current_claim(debatingInfo.modifying_current_claim());
	info.set_current_action(MapDebateAction(debatingInfo.current_debate_action().action_type()));

	rendering_info::ClaimRenderInfo* currentClaim = info.mutable_current_claim();
	currentClaim->set_id(debatingInfo.current_claim().id());
	currentClaim->set_sentence(debatingInfo.current_claim().sentence());
	currentClaim->set_creator_id(debatingInfo.current_claim().creator_id());
	currentClaim->set_status(MapClaimStatus(debatingInfo.current_claim().status()));

	for (int i = 0; i < debatingInfo.children_claims_size(); ++i) {
		const user_engagement::ClaimInfo& childClaim = debatingInfo.children_claims(i);
		rendering_info::ClaimRenderInfo* outChild = info.add_children_claims();
		outChild->set_id(childClaim.id());
		outChild->set_sentence(childClaim.sentence());
		outChild->set_creator_id(childClaim.creator_id());
		outChild->set_status(MapClaimStatus(childClaim.status()));
	}

	for (int i = 0; i < debatingInfo.links_size(); ++i) {
		const user_engagement::LinkInfo& link = debatingInfo.links(i);
		rendering_info::LinkRenderInfo* outLink = info.add_links();
		outLink->set_id(link.id());
		outLink->set_connect_from(link.connect_from());
		outLink->set_connect_to(link.connect_to());
		outLink->set_connection(link.connection());
		outLink->set_creator_id(link.creator_id());
	}

	for (int i = 0; i < debatingInfo.current_challenges_size(); ++i) {
		const user_engagement::ChallengeInfo& challenge = debatingInfo.current_challenges(i);
		rendering_info::ChallengeRenderInfo* outChallenge = info.add_current_challenges();
		outChallenge->set_id(challenge.id());
		outChallenge->set_sentence(challenge.sentence());
		outChallenge->set_creator_id(challenge.creator_id());
		outChallenge->set_status(MapChallengeStatus(challenge.status()));
	}

	if (debatingInfo.has_connecting_info()) {
		const user_engagement::DebatingInfo_ConnectingInfo& srcConnecting = debatingInfo.connecting_info();
		rendering_info::ConnectingRenderInfo* dstConnecting = info.mutable_connecting_info();
		dstConnecting->set_from_claim_id(srcConnecting.from_claim_id());
		dstConnecting->set_to_claim_id(srcConnecting.to_claim_id());
		dstConnecting->set_connecting(srcConnecting.connecting());
		dstConnecting->set_opened_connect_modal(srcConnecting.opened_connect_modal());
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
