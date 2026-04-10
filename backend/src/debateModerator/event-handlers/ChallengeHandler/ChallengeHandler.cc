#include "ChallengeHandler.h"

#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"

void ChallengeHandler::StartChallengeClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set challenging_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo::CurrentDebateAction::CHALLENGING_CLAIM);
    Log::debug("[StartChallengeClaimHandler] Set challenging_claim to true for user: " + std::to_string(user_id));
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ChallengeHandler::CancelChallengeClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set current debate action to VIEWING
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo::CurrentDebateAction::VIEWING_CLAIM);
    // close modal and reset stuff
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->set_opened_challenge_modal(false);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->clear_claim_ids();
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->clear_link_ids();
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->set_challenge_sentence("");
    Log::debug("[CancelChallengeClaimHandler] Set challenging_claim to false for user: " + std::to_string(user_id));
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ChallengeHandler::AddClaimToBeChallenged(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set the challenging_info current_claim to the claim to be challenged
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->add_claim_ids(claim_id);
    Log::debug("[AddClaimToBeChallengedHandler] Set challenging_info current_claim to claim: " + std::to_string(claim_id) + " for user: " + std::to_string(user_id));
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ChallengeHandler::RemoveClaimToBeChallenged(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // remove the claim_id from challenging_info current_claims
    auto* challenging_info = userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info();
    for (int i = 0; i < challenging_info->claim_ids_size(); ++i) {
        if (challenging_info->claim_ids(i) == claim_id) {
            // Swap with last element and remove last
            challenging_info->mutable_claim_ids()->SwapElements(i, challenging_info->claim_ids_size() - 1);
            challenging_info->mutable_claim_ids()->RemoveLast();
            Log::debug("[RemoveClaimToBeChallengedHandler] Removed claim: " + std::to_string(claim_id) + " from challenging_info for user: " + std::to_string(user_id));
            break;
        }
    }
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ChallengeHandler::RemoveLinkToBeChallenged(const int& link_id, const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // remove the link_id from challenging_info current_links
    auto* challenging_info = userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info();
    for (int i = 0; i < challenging_info->link_ids_size(); ++i) {
        if (challenging_info->link_ids(i) == link_id) {
            // Swap with last element and remove last
            challenging_info->mutable_link_ids()->SwapElements(i, challenging_info->link_ids_size() - 1);
            challenging_info->mutable_link_ids()->RemoveLast();
            Log::debug("[RemoveLinkToBeChallengedHandler] Removed link: " + std::to_string(link_id) + " from challenging_info for user: " + std::to_string(user_id));
            break;
        }
    }
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ChallengeHandler::AddLinkToBeChallenged(const int& link_id, const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set the challenging_info current_link to the link to be challenged
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->add_link_ids(link_id);
    Log::debug("[AddLinkToBeChallengedHandler] Set challenging_info current_link to link: " + std::to_string(link_id) + " for user: " + std::to_string(user_id));
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ChallengeHandler::SubmitChallengeClaim(const std::string& challenge_sentence, const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // find the current claim (it's challenging that one)
    int current_claim_id = userProto.engagement().debating_info().current_claim().id();
    int current_debate_id = userProto.engagement().debating_info().debate_id();

    // NEW FLOW: also create a challenge claim in the same debate and connect it
    // to the challenged claim with a CHALLENGE link.
    int same_debate_challenge_claim_id = debateWrapper.createClaim(
        challenge_sentence,
        "challenge claim",
        user_id,
        current_debate_id
    );
    int same_debate_challenge_link_id = -1;

    if (same_debate_challenge_claim_id != -1) {
        same_debate_challenge_link_id = debateWrapper.addLink(
            same_debate_challenge_claim_id,
            current_claim_id,
            "challenge link",
            user_id,
            current_debate_id,
            debate::LinkType::CHALLENGE
        );
        Log::debug(
            "[SubmitChallengeClaimHandler] Added same-debate challenge claim id=" +
            std::to_string(same_debate_challenge_claim_id) +
            " and challenge link id=" + std::to_string(same_debate_challenge_link_id) +
            " for challenged claim id=" + std::to_string(current_claim_id)
        );
    } else {
        Log::warn(
            "[SubmitChallengeClaimHandler] Failed to create same-debate challenge claim for challenged claim id=" +
            std::to_string(current_claim_id)
        );
    }

    // update current claim and all parents to be CHALLENGED
    debate::Claim currentClaim = debateWrapper.getClaimById(current_claim_id);
    while (true) {
        currentClaim.set_status(debate::ClaimStatus::CHALLENGED);
        debateWrapper.updateClaimInDB(currentClaim);
        if (debateWrapper.isRoot(currentClaim.id())) {
            break; // reached root
        }
        currentClaim = debateWrapper.findClaimParent(currentClaim.id());
    }

    // close the challenging modal and reset stuff
    CancelChallengeClaim(user_id, debateWrapper);
    CloseAddChallenge(user_id, debateWrapper);
    Log::debug("[SubmitChallengeClaimHandler] Submitted new challenge for user: " + std::to_string(user_id));
}

void ChallengeHandler::ConcedeChallenge(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // Legacy challenge protobuf records are deprecated. Concede now only
    // clears the in-progress challenge interaction state for the user.
    
    // clear the user's challenging state
    CancelChallengeClaim(user_id, debateWrapper);
    CloseAddChallenge(user_id, debateWrapper);
    
    Log::debug("[ConcedeChallengeHandler] User: " + std::to_string(user_id) + " conceded challenge");
}

void ChallengeHandler::OpenAddChallenge(const int& user_id, DebateWrapper& debateWrapper) {
    // find user protobuf
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->set_opened_challenge_modal(true);
    debateWrapper.updateUserProtobuf(user_id, userProto);
    Log::debug("[OpenAddChallengeHandler] OpenAddChallenge: set challenging_claim to true for user: " + std::to_string(user_id));
}

void ChallengeHandler::CloseAddChallenge(const int& user_id, DebateWrapper& debateWrapper) {
    // find user protobuf
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->set_opened_challenge_modal(false);
    debateWrapper.updateUserProtobuf(user_id, userProto);
    Log::debug("[CloseAddChallengeHandler] CloseAddChallenge: set challenging_claim to false for user: " + std::to_string(user_id));
}

void ChallengeHandler::DeleteChallenge(const int& challenge_id, const int& user_id, DebateWrapper& debateWrapper) {
    // challenge_id now refers to challenge claim id
    const int challenge_claim_id = challenge_id;
    debate::Claim challengeClaim = debateWrapper.getClaimById(challenge_claim_id);
    if (challengeClaim.id() == 0) {
        Log::warn("[DeleteChallengeHandler] Challenge claim ID " + std::to_string(challenge_claim_id) + " not found.");
        return;
    }

    if (challengeClaim.creator_id() != user_id) {
        Log::warn("[DeleteChallengeHandler] User: " + std::to_string(user_id) + " attempted to delete challenge claim ID: " + std::to_string(challenge_claim_id) + " but is not the creator.");
        return;
    }

    int challengeLinkId = -1;
    int challengedClaimId = -1;
    for (int i = 0; i < challengeClaim.link_ids_size(); ++i) {
        const int linkId = challengeClaim.link_ids(i);
        debate::Link linkProto = debateWrapper.getLinkById(linkId);
        if (linkProto.link_type() == debate::LinkType::CHALLENGE && linkProto.connect_from() == challenge_claim_id) {
            challengeLinkId = linkId;
            challengedClaimId = linkProto.connect_to();
            break;
        }
    }

    if (challengeLinkId != -1) {
        debateWrapper.deleteLinkById(challengeLinkId);
        Log::debug("[DeleteChallengeHandler] Removed challenge link ID: " + std::to_string(challengeLinkId) + " from challenge claim ID: " + std::to_string(challenge_claim_id));
    } else {
        Log::warn("[DeleteChallengeHandler] No CHALLENGE link found from challenge claim ID: " + std::to_string(challenge_claim_id));
    }

    Log::debug("[DeleteChallengeHandler] Deleted challenge claim ID: " + std::to_string(challenge_claim_id) + " for user: " + std::to_string(user_id));
}

