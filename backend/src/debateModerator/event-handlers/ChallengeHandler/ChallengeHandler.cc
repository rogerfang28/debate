#include "ChallengeHandler.h"
#include "../../../utils/BasisRegistry.h"

#include <iostream>
#include <set>
#include "debate.pb.h"
#include "user.pb.h"
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

    // set challenging_claim to true and open the challenge modal directly --
    // previously this was a separate step (openCreateChallengeButton), but
    // one click should be enough to get to the challenge form.
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo::CurrentDebateAction::CHALLENGING_CLAIM);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->set_opened_challenge_modal(true);
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

    // Putting a link into a challenge when no challenge is under way STARTS
    // one, modal open, with the link already picked. That is what the
    // selected-argument panel's Challenge button means; making it two clicks
    // (start, then pick) would only be ceremony.
    auto* debating = userProto.mutable_engagement()->mutable_debating_info();
    if (debating->current_debate_action().action_type() !=
        user_engagement::DebatingInfo::CurrentDebateAction::CHALLENGING_CLAIM) {
        debating->mutable_current_debate_action()->set_action_type(
            user_engagement::DebatingInfo::CurrentDebateAction::CHALLENGING_CLAIM);
        debating->mutable_challenging_info()->set_opened_challenge_modal(true);
        Log::debug("[AddLinkToBeChallengedHandler] Started a challenge for user: " + std::to_string(user_id));
    }

    // set the challenging_info current_link to the link to be challenged
    userProto.mutable_engagement()->mutable_debating_info()->mutable_challenging_info()->add_link_ids(link_id);
    Log::debug("[AddLinkToBeChallengedHandler] Set challenging_info current_link to link: " + std::to_string(link_id) + " for user: " + std::to_string(user_id));
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ChallengeHandler::SubmitChallengeClaim(const std::vector<std::string>& challenge_sentences, const std::vector<std::string>& descriptions, const std::string& basis_in, const int& user_id, DebateWrapper& debateWrapper, int voice_id, const debate::Source* source) {
    // Same rule as AddClaimUnderClaim: only a basis the registry knows is stored.
    const std::string basis = bases::Sanitize(basis_in);
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // find the current claim (it's challenging that one)
    int current_claim_id = userProto.engagement().debating_info().current_claim().id();
    int current_debate_id = userProto.engagement().debating_info().debate_id();

    // Several sentences = ONE opposing relation with several from-claims:
    // "these together refute that", not N independent challenges. The first
    // row establishes the relation id; the rest join it.
    //
    // The link's connection carries the basis, matching how support relations
    // store it. Empty falls back to the old fixed "challenge link" string so
    // existing rows and callers are unaffected.
    const std::string connection = basis.empty() ? "challenge link" : basis;

    // A LINK in the challenge list means the target is an ARGUMENT, not a
    // claim: "your evidence is real, but it does not prove that". The row is
    // aimed at that relation, and CLAIM_ID_TO holds the claim the argument
    // was for so readers that know only claims still draw the challenge
    // beside the right claim. One challenge attacks one thing; if several
    // links were collected the first is the target and the rest are ignored
    // with a warning, since silently picking the last would be worse.
    int target_relation_id = 0;
    int argument_target_claim_id = 0;
    {
        const auto& challenging = userProto.engagement().debating_info().challenging_info();
        if (challenging.link_ids_size() > 0) {
            const debate::Relationship targetRel = debateWrapper.getLinkById(challenging.link_ids(0));
            if (targetRel.id() != 0) {
                target_relation_id = targetRel.id();
                argument_target_claim_id =
                    targetRel.to_case() == debate::Relationship::kToClaimId
                        ? targetRel.to_claim_id() : targetRel.link().connect_to();
                if (challenging.link_ids_size() > 1) {
                    Log::warn("[SubmitChallengeClaimHandler] " +
                              std::to_string(challenging.link_ids_size()) +
                              " links collected; a challenge attacks one argument. Using relation " +
                              std::to_string(target_relation_id));
                }
            } else {
                Log::warn("[SubmitChallengeClaimHandler] Link " +
                          std::to_string(challenging.link_ids(0)) +
                          " not found; challenging the current claim instead");
            }
        }
    }
    const int challenged_claim_for_row =
        target_relation_id > 0 ? argument_target_claim_id : current_claim_id;

    int same_debate_challenge_claim_id = -1;
    int same_debate_challenge_link_id = -1;
    int challenge_relation_id = 0;

    // Each challenge claim carries its OWN description, at the same index.
    for (size_t i = 0; i < challenge_sentences.size(); ++i) {
        const std::string& sentence = challenge_sentences[i];
        const std::string description =
            i < descriptions.size() && !descriptions[i].empty()
                ? descriptions[i] : std::string("challenge claim");
        if (sentence.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;  // blank box
        }
        const int claim_id = debateWrapper.createClaim(
            sentence,
            description,
            user_id,
            current_debate_id,
            voice_id,
            source
        );
        if (claim_id == -1) {
            continue;
        }

        const int link_id = debateWrapper.addLink(
            claim_id,
            challenged_claim_for_row,
            connection,
            user_id,
            current_debate_id,
            debate::LinkType::CHALLENGE,
            challenge_relation_id,
            target_relation_id
        );

        if (challenge_relation_id == 0) {
            challenge_relation_id = link_id;  // its own relation; the rest join
        }
        if (same_debate_challenge_claim_id == -1) {
            // Downstream code (cursor placement, concede lookup) expects a
            // single challenge claim; the first is the one it means.
            same_debate_challenge_claim_id = claim_id;
            same_debate_challenge_link_id = link_id;
        }
    }

    if (same_debate_challenge_claim_id != -1) {
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

    // No status is written here. The challenger's view -- the challenged
    // claim is FALSE for them, or for the voice they named; nothing at all
    // for an undercut -- is the OPPOSE move the logger appends for this
    // event, and the page replays the log.

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
    const debate::Relationship challengeRel = debateWrapper.getLinkById(challenge_link_id);
    const debate::Relationship::Link challengeLink = challengeRel.link();
    // An undercut attacks an argument, not the claim in connect_to().
    const bool undercutsArgument = challengeRel.to_case() == debate::Relationship::kToRelationId;
    if (challenge_link_id <= 0 || challengeLink.id() == 0 || challengeLink.link_type() != debate::LinkType::CHALLENGE) {
        Log::error("[ConcedeChallengeHandler] Cannot find challenge with id " + std::to_string(challenge_link_id));
        CancelChallengeClaim(user_id, debateWrapper);
        CloseAddChallenge(user_id, debateWrapper);
        return;
    }

    int challengeClaimId = challengeLink.connect_from();
    int challengedClaimId = challengeLink.connect_to();
    Log::debug("[ConcedeChallengeHandler] Found challenge link " + std::to_string(challenge_link_id) +
              ": from=" + std::to_string(challengeClaimId) + " to=" + std::to_string(challengedClaimId));

    // No status is written here. What conceding means for the conceder --
    // the challenge and everything under it TRUE, the challenged claim FALSE
    // unless the challenge was an undercut -- is the CONCEDE move the logger
    // appends for this event, and the page replays the log.

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
    debate::Relationship::Link outgoingChallenge = debateWrapper.findOutgoingChallengeLink(challenge_claim_id).link();
    if (outgoingChallenge.link_type() == debate::LinkType::CHALLENGE) {
        challengeLinkId = outgoingChallenge.id();
        challengedClaimId = outgoingChallenge.connect_to();
    }

    if (challengeLinkId != -1) {
        debateWrapper.deleteLinkById(challengeLinkId);
        Log::debug("[DeleteChallengeHandler] Removed challenge link ID: " + std::to_string(challengeLinkId) + " from challenge claim ID: " + std::to_string(challenge_claim_id));
    } else {
        Log::warn("[DeleteChallengeHandler] No CHALLENGE link found from challenge claim ID: " + std::to_string(challenge_claim_id));
    }

    Log::debug("[DeleteChallengeHandler] Deleted challenge claim ID: " + std::to_string(challenge_claim_id) + " for user: " + std::to_string(user_id));
}

