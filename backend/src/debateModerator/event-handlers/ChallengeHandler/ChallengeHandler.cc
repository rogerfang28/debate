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

    // No status updates needed here — the cascade in UpdateStatusOfAllClaimsInDebate
    // handles status based on challenge claim status.

    // Record the challenger's view: they think the challenged claim is FALSE
    debate::Claim challengedClaim = debateWrapper.getClaimById(current_claim_id);
    user::User challengerProto = debateWrapper.getUserProtobuf(user_id);
    (*challengedClaim.mutable_user_statuses())[challengerProto.username()] = debate::ClaimStatus::FALSE_CLAIM;
    debateWrapper.updateClaimInDB(challengedClaim);

    // close the challenging modal and reset stuff
    CancelChallengeClaim(user_id, debateWrapper);
    CloseAddChallenge(user_id, debateWrapper);

    // Set the user's current claim to the newly created challenge claim
    // so that subsequent operations (like CONCEDE_CHALLENGE) can find it.
    if (same_debate_challenge_claim_id != -1) {
        user::User updatedUser = debateWrapper.getUserProtobuf(user_id);
        updatedUser.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(same_debate_challenge_claim_id);
        debateWrapper.updateUserProtobuf(user_id, updatedUser);
    }

    Log::debug("[SubmitChallengeClaimHandler] Submitted new challenge for user: " + std::to_string(user_id));
}

void ChallengeHandler::ConcedeChallenge(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    std::string username = userProto.username();

    // Find the user's current claim — this is the challenge claim being conceded.
    int challengeClaimId = userProto.engagement().debating_info().current_claim().id();
    if (challengeClaimId == 0) {
        Log::warn("[ConcedeChallengeHandler] User " + std::to_string(user_id) + " has no current claim");
        CancelChallengeClaim(user_id, debateWrapper);
        CloseAddChallenge(user_id, debateWrapper);
        return;
    }

    debate::Claim challengeClaim = debateWrapper.getClaimById(challengeClaimId);
    if (challengeClaim.id() == 0) {
        Log::warn("[ConcedeChallengeHandler] Challenge claim " + std::to_string(challengeClaimId) + " not found");
        CancelChallengeClaim(user_id, debateWrapper);
        CloseAddChallenge(user_id, debateWrapper);
        return;
    }

    // Find the CHALLENGE link from this claim to the challenged claim.
    int challengedClaimId = -1;
    for (int i = 0; i < challengeClaim.link_ids_size(); ++i) {
        int linkId = challengeClaim.link_ids(i);
        debate::Link link = debateWrapper.getLinkById(linkId);
        if (link.link_type() == debate::LinkType::CHALLENGE && link.connect_from() == challengeClaimId) {
            challengedClaimId = link.connect_to();
            break;
        }
    }

    if (challengedClaimId == -1) {
        Log::warn("[ConcedeChallengeHandler] No CHALLENGE link found from claim " + std::to_string(challengeClaimId));
        CancelChallengeClaim(user_id, debateWrapper);
        CloseAddChallenge(user_id, debateWrapper);
        return;
    }

    // Step 1: Mark the challenge claim as FALSE for the concessor (they conceded it).
    debate::Claim challengeClaimUpdated = debateWrapper.getClaimById(challengeClaimId);
    (*challengeClaimUpdated.mutable_user_statuses())[username] = debate::ClaimStatus::FALSE_CLAIM;
    debateWrapper.updateClaimInDB(challengeClaimUpdated);
    Log::debug("[ConcedeChallengeHandler] User " + username + " conceded challenge claim " +
              std::to_string(challengeClaimId) + " → FALSE_CLAIM");

    // Step 2: The challenged claim is TRUE (the challenge succeeded).
    // Then find what the challenged claim itself challenges (if anything),
    // and propagate FALSE from there up the parent chain.
    //
    // Example: A concedes claim 5 (challenges claim 4).
    //   Claim 5 → FALSE (conceded)
    //   Claim 4 → TRUE (challenge succeeded)
    //   Claim 2 → FALSE (claim 4 challenges claim 2, and 4 succeeded)
    //   Claim 1 → FALSE (parent of 2, propagated up)

    // Set the challenged claim to TRUE
    debate::Claim challengedClaim = debateWrapper.getClaimById(challengedClaimId);
    (*challengedClaim.mutable_user_statuses())[username] = debate::ClaimStatus::TRUE_CLAIM;
    debateWrapper.updateClaimInDB(challengedClaim);
    Log::debug("[ConcedeChallengeHandler] Set claim " +
              std::to_string(challengedClaimId) + " as TRUE for " + username);

    // Find the claim that the challenged claim itself challenges (via CHALLENGE link).
    int deeperChallengedId = -1;
    for (int i = 0; i < challengedClaim.link_ids_size(); ++i) {
        int linkId = challengedClaim.link_ids(i);
        debate::Link link = debateWrapper.getLinkById(linkId);
        if (link.link_type() == debate::LinkType::CHALLENGE && link.connect_from() == challengedClaimId) {
            deeperChallengedId = link.connect_to();
            break;
        }
    }

    // Walk up the parent chain from the deeper-challenged claim (if any),
    // propagating FALSE to it and all its ancestors.
    // If the challenged claim doesn't itself challenge anything, walk up from it.
    int walkStartId = (deeperChallengedId != -1) ? deeperChallengedId : challengedClaimId;

    // If we started from the challenged claim itself (no deeper challenge),
    // we already set it to TRUE, so skip it and just walk ancestors.
    // If we started from the deeper-challenged claim, we need to set it to FALSE.
    bool alreadyHandledFirst = (deeperChallengedId == -1);

    int currentClaimId = walkStartId;
    while (currentClaimId != 0) {
        debate::Claim currentClaim = debateWrapper.getClaimById(currentClaimId);
        if (currentClaim.id() == 0) break;

        if (alreadyHandledFirst) {
            alreadyHandledFirst = false;
        } else {
            (*currentClaim.mutable_user_statuses())[username] = debate::ClaimStatus::FALSE_CLAIM;
            debateWrapper.updateClaimInDB(currentClaim);
            Log::debug("[ConcedeChallengeHandler] Propagated FALSE to claim " +
                      std::to_string(currentClaimId) + " for " + username);
        }

        // Move up to parent via PARENT_CHILD link
        debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
        if (parentClaim.id() == 0 || parentClaim.id() == currentClaimId) {
            break;
        }
        currentClaimId = parentClaim.id();
    }

    // Also update global claim statuses
    int debateId = userProto.engagement().debating_info().debate_id();
    if (debateId > 0) {
        debateWrapper.UpdateStatusOfAllClaimsInDebate(debateId);
    }

    // Step 3: Find all claims that challenge the conceded claim (CHALLENGE link → conceded claim).
    // When a claim is conceded, any challenge claim targeting it is vindicated (TRUE for concessor).
    // Example: A concedes claim 5 (challenging claim 4). Claim 6 challenges claim 5.
    //   Claim 6 → TRUE for A (it correctly challenged the now-conceded claim 5).
    // Then recursively apply: if claim 6 is now TRUE, any challenge on claim 6 is FALSE.
    std::vector<int> downstreamChallengeIds;
    if (debateId > 0) {
        auto allLinks = debateWrapper.getLinksForDebate(debateId);
        for (const auto& linkTuple : allLinks) {
            int fromClaimId = std::get<1>(linkTuple);
            int toClaimId = std::get<2>(linkTuple);
            debate::Link link = debateWrapper.getLinkById(std::get<0>(linkTuple));
            if (link.link_type() == debate::LinkType::CHALLENGE && toClaimId == challengeClaimId) {
                downstreamChallengeIds.push_back(fromClaimId);
            }
        }
    }

    // Process downstream challenge claims:
    // For each claim that challenges the conceded claim, set it to TRUE for the concessor.
    // Then recursively find challenges targeting it and set those to FALSE.
    for (int downstreamId : downstreamChallengeIds) {
        debate::Claim downstreamClaim = debateWrapper.getClaimById(downstreamId);
        if (downstreamClaim.id() == 0) continue;
        (*downstreamClaim.mutable_user_statuses())[username] = debate::ClaimStatus::TRUE_CLAIM;
        debateWrapper.updateClaimInDB(downstreamClaim);
        Log::debug("[ConcedeChallengeHandler] Set downstream challenge claim " +
                  std::to_string(downstreamId) + " as TRUE for " + username);

        // Recursively: find challenges targeting this downstream claim → those are FALSE
        // (because the downstream claim succeeded, so challenges against it failed)
        std::vector<int> furtherDownstream;
        if (debateId > 0) {
            auto allLinks2 = debateWrapper.getLinksForDebate(debateId);
            for (const auto& linkTuple : allLinks2) {
                int fromClaimId2 = std::get<1>(linkTuple);
                int toClaimId2 = std::get<2>(linkTuple);
                debate::Link link2 = debateWrapper.getLinkById(std::get<0>(linkTuple));
                if (link2.link_type() == debate::LinkType::CHALLENGE && toClaimId2 == downstreamId) {
                    furtherDownstream.push_back(fromClaimId2);
                }
            }
        }
        for (int fdId : furtherDownstream) {
            if (fdId == downstreamId) continue;  // avoid infinite loop
            debate::Claim fdClaim = debateWrapper.getClaimById(fdId);
            if (fdClaim.id() == 0) continue;
            (*fdClaim.mutable_user_statuses())[username] = debate::ClaimStatus::FALSE_CLAIM;
            debateWrapper.updateClaimInDB(fdClaim);
            Log::debug("[ConcedeChallengeHandler] Recursively set further-downstream claim " +
                      std::to_string(fdId) + " as FALSE for " + username);
        }
    }

    // clear the user's challenging state
    CancelChallengeClaim(user_id, debateWrapper);
    CloseAddChallenge(user_id, debateWrapper);

    Log::debug("[ConcedeChallengeHandler] User: " + std::to_string(user_id) + " conceded challenge on claim " +
              std::to_string(challengeClaimId) + " challenging " + std::to_string(challengedClaimId));
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

