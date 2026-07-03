#include "ChallengeHandler.h"

#include <iostream>
#include <set>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"

static std::string claimStatusToString(debate::ClaimStatus s) {
    if (s == debate::ClaimStatus::TRUE_CLAIM) return "TRUE_CLAIM";
    if (s == debate::ClaimStatus::FALSE_CLAIM) return "FALSE_CLAIM";
    return "UNDETERMINED";
}

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

    // Propagate statuses through the graph (per-user cascade)
    debateWrapper.PropagateClaimStatuses(current_debate_id);

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

void ChallengeHandler::ConcedeChallenge(const int& user_id, const int& challenge_link_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    std::string username = userProto.username();

    int debateId = userProto.engagement().debating_info().debate_id();
    if (debateId == 0) {
        Log::warn("[ConcedeChallengeHandler] User " + std::to_string(user_id) + " is not in a debate");
        CancelChallengeClaim(user_id, debateWrapper);
        CloseAddChallenge(user_id, debateWrapper);
        return;
    }

    // Use the challenge_link_id to directly look up the challenge link.
    // The link's connect_from = challenge claim ID, connect_to = challenged claim ID.
    int challengeClaimId = -1;
    int challengedClaimId = -1;

    if (challenge_link_id > 0) {
        debate::Relationship::Link challengeLink = debateWrapper.getLinkById(challenge_link_id).link();
        if (challengeLink.id() != 0 && challengeLink.link_type() == debate::LinkType::CHALLENGE) {
            challengeClaimId = challengeLink.connect_from();
            challengedClaimId = challengeLink.connect_to();
            Log::debug("[ConcedeChallengeHandler] Found challenge link " + std::to_string(challenge_link_id) +
                      ": from=" + std::to_string(challengeClaimId) + " to=" + std::to_string(challengedClaimId));
        } else {
            Log::warn("[ConcedeChallengeHandler] challenge_link_id " + std::to_string(challenge_link_id) +
                      " is not a valid CHALLENGE link");
        }
    }

    // Fallback: if link lookup failed, use the old scan approach
    if (challengeClaimId == -1) {
        int bestFromId = -1;
        auto allLinks = debateWrapper.getLinksForDebate(debateId);
        for (const auto& linkTuple : allLinks) {
            int fromClaimId = std::get<1>(linkTuple);
            int toClaimId = std::get<2>(linkTuple);
            int linkType = std::get<5>(linkTuple);
            if (linkType == static_cast<int>(debate::LinkType::CHALLENGE)) {
                debate::Claim targetClaim = debateWrapper.getClaimById(toClaimId);
                if (targetClaim.id() != 0 && targetClaim.creator_id() == user_id) {
                    if (fromClaimId > bestFromId) {
                        bestFromId = fromClaimId;
                        challengeClaimId = fromClaimId;
                        challengedClaimId = toClaimId;
                    }
                }
            }
        }
    }

    // Fallback: current claim check
    if (challengeClaimId == -1) {
        int currentClaimId = userProto.engagement().debating_info().current_claim().id();
        if (currentClaimId == 0) {
            Log::warn("[ConcedeChallengeHandler] User " + std::to_string(user_id) + " has no current claim");
            CancelChallengeClaim(user_id, debateWrapper);
            CloseAddChallenge(user_id, debateWrapper);
            return;
        }
        debate::Claim currentClaim = debateWrapper.getClaimById(currentClaimId);
        if (currentClaim.id() == 0) {
            Log::warn("[ConcedeChallengeHandler] Current claim " + std::to_string(currentClaimId) + " not found");
            CancelChallengeClaim(user_id, debateWrapper);
            CloseAddChallenge(user_id, debateWrapper);
            return;
        }
        // Check if current claim is a challenge (has outgoing CHALLENGE link)
        for (int i = 0; i < currentClaim.link_ids_size(); ++i) {
            int linkId = currentClaim.link_ids(i);
            debate::Relationship::Link link = debateWrapper.getLinkById(linkId).link();
            if (link.link_type() == debate::LinkType::CHALLENGE && link.connect_from() == currentClaimId) {
                challengeClaimId = currentClaimId;
                challengedClaimId = link.connect_to();
                break;
            }
        }
    }

    if (challengeClaimId == -1) {
        Log::warn("[ConcedeChallengeHandler] No challenge found for user " + std::to_string(user_id) + " to concede to");
        CancelChallengeClaim(user_id, debateWrapper);
        CloseAddChallenge(user_id, debateWrapper);
        return;
    }

    // Step 1: Mark the challenge claim as TRUE_CLAIM for the concessor.
    // When User A concedes, they admit the challenge was valid → challenge claim is TRUE for them.
    debate::Claim challengeClaimUpdated = debateWrapper.getClaimById(challengeClaimId);
    (*challengeClaimUpdated.mutable_user_statuses())[username] = debate::ClaimStatus::TRUE_CLAIM;
    debateWrapper.updateClaimInDB(challengeClaimUpdated);
    Log::debug("[ConcedeChallengeHandler] User " + username + " conceded challenge claim " +
              std::to_string(challengeClaimId) + " → TRUE_CLAIM");

    // Step 1b: Mark the challenged claim (the original claim being attacked) as FALSE_CLAIM
    // for the concessor — they admit their own claim was wrong.
    debate::Claim challengedClaimUpdated = debateWrapper.getClaimById(challengedClaimId);
    if (challengedClaimUpdated.id() != 0) {
        (*challengedClaimUpdated.mutable_user_statuses())[username] = debate::ClaimStatus::FALSE_CLAIM;
        debateWrapper.updateClaimInDB(challengedClaimUpdated);
        Log::debug("[ConcedeChallengeHandler] User " + username + " concedes challenged claim " +
                  std::to_string(challengedClaimId) + " → FALSE_CLAIM");
    }

    // Step 1c: Mark all PARENT_CHILD descendants of the challenge claim as TRUE for concessor.
    // When user concedes, they agree the entire challenge chain is correct —
    // not just the root challenge claim, but every child (evidence) under it.
    {
        std::set<int> visited;
        std::vector<int> queue = {challengeClaimId};
        visited.insert(challengeClaimId);
        while (!queue.empty()) {
            int currentId = queue.back();
            queue.pop_back();
            auto allLinksForChildren = debateWrapper.getLinksForDebate(debateId);
            for (const auto& linkTuple : allLinksForChildren) {
                int fromId = std::get<1>(linkTuple);
                int toId = std::get<2>(linkTuple);
                int lType = std::get<5>(linkTuple);
                if (lType == static_cast<int>(debate::LinkType::PARENT_CHILD) && fromId == currentId) {
                    if (visited.find(toId) == visited.end()) {
                        visited.insert(toId);
                        debate::Claim childClaim = debateWrapper.getClaimById(toId);
                        if (childClaim.id() != 0) {
                            (*childClaim.mutable_user_statuses())[username] = debate::ClaimStatus::TRUE_CLAIM;
                            debateWrapper.updateClaimInDB(childClaim);
                            Log::debug("[ConcedeChallengeHandler] Child claim " + std::to_string(toId) +
                                      " → TRUE for " + username);
                        }
                        queue.push_back(toId);
                    }
                }
            }
        }
    }

    // Step 2: Find downstream challenges of the conceded claim and set them to TRUE.
    // When A concedes to challenge 8 (targeting 7), any claim that challenges 8 is vindicated.
    if (debateId > 0) {
        auto allLinks2 = debateWrapper.getLinksForDebate(debateId);
        for (const auto& linkTuple : allLinks2) {
            int fromClaimId = std::get<1>(linkTuple);
            int toClaimId = std::get<2>(linkTuple);
            int linkType = std::get<5>(linkTuple);
            if (linkType == static_cast<int>(debate::LinkType::CHALLENGE) && toClaimId == challengeClaimId) {
                // fromClaimId challenges the conceded claim → vindicated
                debate::Claim downstreamClaim = debateWrapper.getClaimById(fromClaimId);
                if (downstreamClaim.id() != 0) {
                    (*downstreamClaim.mutable_user_statuses())[username] = debate::ClaimStatus::TRUE_CLAIM;
                    debateWrapper.updateClaimInDB(downstreamClaim);
                    Log::debug("[ConcedeChallengeHandler] Downstream challenge " + std::to_string(fromClaimId) +
                              " → TRUE for " + username);
                }
            }
        }
    }

    // Step 3: Propagate — cascades status changes to all affected users.
    if (debateId > 0) {
        std::set<int> conceded_claims = {challengeClaimId};
        debateWrapper.PropagateClaimStatuses(debateId, conceded_claims);
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
        debate::Relationship::Link linkProto = debateWrapper.getLinkById(linkId).link();
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

