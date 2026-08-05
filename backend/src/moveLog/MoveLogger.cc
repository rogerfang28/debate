#include "MoveLogger.h"
#include "../utils/Log.h"
#include "user.pb.h"

#include <algorithm>
#include <string>

namespace {

// Appends one move, swallowing every failure. The log runs in parallel with a
// working system; it must never be able to break it.
void append(DatabaseWrapper& db,
            int debateId,
            int actorId,
            const std::string& type,
            bool targetIsClaim,
            int targetId,
            const std::string& payload = "") {
    if (debateId <= 0 || targetId <= 0) {
        // A move we cannot place in a debate, or that points at nothing, would
        // be worse than no move at all: it would look like a real part of some
        // debate's sequence. Refuse it loudly instead.
        Log::warn("[MoveLogger] Skipping '" + type + "' move: debate=" +
                  std::to_string(debateId) + " target=" + std::to_string(targetId));
        return;
    }
    try {
        if (!db.moves.appendMove(debateId, actorId, type,
                                 targetIsClaim ? "claim" : "relation",
                                 targetId, payload)) {
            Log::warn("[MoveLogger] Failed to append '" + type + "' move for debate " +
                      std::to_string(debateId) + "; originating operation unaffected.");
        }
    } catch (const std::exception& e) {
        Log::error("[MoveLogger] Append threw (" + std::string(e.what()) +
                   "); originating operation unaffected.");
    } catch (...) {
        Log::error("[MoveLogger] Append threw unknown exception; "
                   "originating operation unaffected.");
    }
}

int currentClaimIdOf(const user::User& u) {
    return u.engagement().debating_info().current_claim().id();
}

int debateIdOf(const user::User& u) {
    return u.engagement().debating_info().debate_id();
}

// Finds the relation joining two claims in that direction. Used to locate a
// relation the handler just created, since handlers do not report their ids.
// Returns 0 if there is none.
int findRelationBetween(DebateWrapper& debateWrapper, int fromClaimId, int toClaimId) {
    if (fromClaimId <= 0 || toClaimId <= 0) {
        return 0;
    }
    for (int linkId : debateWrapper.findLinksUnder(fromClaimId)) {
        debate::Relationship::Link link = debateWrapper.getLinkById(linkId).link();
        if (link.id() != 0 &&
            link.connect_from() == fromClaimId &&
            link.connect_to() == toClaimId) {
            return link.id();
        }
    }
    return 0;
}

}  // namespace

namespace MoveLogger {

PreState capture(const debate_event::DebateEvent& event,
                 const int& user_id,
                 DebateWrapper& debateWrapper) {
    PreState pre;

    switch (event.type()) {
        // --- Creations: record the parent so the new child can be found by
        // difference once the handler has run.
        case debate_event::ADD_CHILD_CLAIM: {
            user::User u = debateWrapper.getUserProtobuf(user_id);
            pre.debateId = debateIdOf(u);
            pre.parentClaimId = currentClaimIdOf(u);
            pre.childrenBefore = debateWrapper.findChildrenIds(pre.parentClaimId);
            pre.relevant = true;
            break;
        }

        // The challenged claim is wherever the user is standing now; the
        // handler moves their cursor to the new challenge claim afterwards.
        case debate_event::SUBMIT_CHALLENGE_CLAIM: {
            user::User u = debateWrapper.getUserProtobuf(user_id);
            pre.debateId = debateIdOf(u);
            pre.targetClaimId = currentClaimIdOf(u);
            pre.relevant = true;
            break;
        }

        // --- Concede: resolve the challenge before any cascade touches it.
        case debate_event::CONCEDE_CHALLENGE: {
            user::User u = debateWrapper.getUserProtobuf(user_id);
            pre.debateId = debateIdOf(u);
            pre.challengeRelationId = event.concede_challenge().challenge_link_id();
            debate::Relationship::Link link =
                debateWrapper.getLinkById(pre.challengeRelationId).link();
            pre.challengingClaimId = link.connect_from();
            pre.challengedClaimId = link.connect_to();
            pre.relevant = true;
            break;
        }

        // --- Deletions: everything below is unreadable once the row is gone.
        case debate_event::DELETE_CURRENT_STATEMENT: {
            user::User u = debateWrapper.getUserProtobuf(user_id);
            pre.targetClaimId = currentClaimIdOf(u);
            pre.debateId = debateWrapper.findDebateId(pre.targetClaimId);
            pre.relevant = true;
            break;
        }

        case debate_event::DELETE_CHILD_CLAIM: {
            pre.targetClaimId = event.delete_child_claim().claim_id();
            pre.debateId = debateWrapper.findDebateId(pre.targetClaimId);
            pre.relevant = true;
            break;
        }

        case debate_event::DELETE_LINK: {
            pre.targetRelationId = event.delete_link().link_id();
            debate::Relationship::Link link =
                debateWrapper.getLinkById(pre.targetRelationId).link();
            pre.debateId = link.debate_id();
            pre.relevant = true;
            break;
        }

        case debate_event::DELETE_CHALLENGE: {
            pre.targetClaimId = event.delete_challenge().challenge_id();
            debate::Claim c = debateWrapper.getClaimById(pre.targetClaimId);
            pre.debateId = c.debate_id();
            pre.relevant = true;
            break;
        }

        // --- Connecting two existing claims: remember the endpoints so the
        // new relation can be located afterwards.
        case debate_event::SUBMIT_CONNECT_CLAIMS: {
            user::User u = debateWrapper.getUserProtobuf(user_id);
            const auto& ci = u.engagement().debating_info().connecting_info();
            pre.parentClaimId = ci.from_claim_id();
            pre.targetClaimId = ci.to_claim_id();
            pre.debateId = debateWrapper.findDebateId(pre.parentClaimId);
            pre.relevant = true;
            break;
        }

        // AddDebate creates the debate but does NOT move the user into it, so
        // the user's engagement still points at wherever they were. Snapshot
        // their debate list and find the new one by difference afterwards.
        case debate_event::CREATE_DEBATE:
            pre.debateIdsBefore = debateWrapper.getUserDebateIds(user_id);
            pre.relevant = true;
            break;

        default:
            // Everything else is UI state, navigation, or an action with no
            // clean move type. See the Step 3 notes.
            break;
    }

    return pre;
}

void logForEvent(const debate_event::DebateEvent& event,
                 const int& user_id,
                 const PreState& pre,
                 DebateWrapper& debateWrapper,
                 DatabaseWrapper& databaseWrapper) {
    if (!pre.relevant) {
        return;
    }

    switch (event.type()) {
        case debate_event::CREATE_DEBATE: {
            // Find the debate that appeared. Cannot read this from the user:
            // creating a debate does not move them into it.
            std::vector<int> after = debateWrapper.getUserDebateIds(user_id);
            int newDebateId = 0;
            for (int id : after) {
                if (std::find(pre.debateIdsBefore.begin(), pre.debateIdsBefore.end(), id) ==
                    pre.debateIdsBefore.end()) {
                    newDebateId = id;
                    break;
                }
            }
            if (newDebateId == 0) {
                Log::warn("[MoveLogger] CREATE_DEBATE produced no new debate; nothing logged.");
                break;
            }

            // The root claim is recorded on the debate itself.
            std::vector<uint8_t> raw = debateWrapper.getDebateProtobuf(newDebateId);
            debate::Debate debateProto;
            if (raw.empty() || !debateProto.ParseFromArray(raw.data(), raw.size())) {
                Log::warn("[MoveLogger] Could not read debate " + std::to_string(newDebateId) +
                          " to find its root claim; nothing logged.");
                break;
            }

            append(databaseWrapper, newDebateId, user_id, "ASSERT", true,
                   debateProto.root_claim_id());
            break;
        }

        case debate_event::ADD_CHILD_CLAIM: {
            // Identify the new claim by difference rather than by assuming it
            // is the highest id -- another user may be adding claims too.
            std::vector<int> after = debateWrapper.findChildrenIds(pre.parentClaimId);
            int newClaimId = 0;
            for (int id : after) {
                if (std::find(pre.childrenBefore.begin(), pre.childrenBefore.end(), id) ==
                    pre.childrenBefore.end()) {
                    newClaimId = id;
                    break;
                }
            }
            if (newClaimId == 0) {
                Log::warn("[MoveLogger] ADD_CHILD_CLAIM produced no new child under " +
                          std::to_string(pre.parentClaimId) + "; nothing logged.");
                break;
            }

            append(databaseWrapper, pre.debateId, user_id, "ASSERT", true, newClaimId);

            // The PARENT_CHILD relation that carries it.
            const int relationId = findRelationBetween(debateWrapper, pre.parentClaimId, newClaimId);
            if (relationId > 0) {
                append(databaseWrapper, pre.debateId, user_id, "ASSERT", false, relationId,
                       "{\"from_claim_id\":" + std::to_string(pre.parentClaimId) +
                       ",\"to_claim_id\":" + std::to_string(newClaimId) + "}");
            }
            break;
        }

        case debate_event::SUBMIT_CHALLENGE_CLAIM: {
            // The handler leaves the user standing on the claim it just made.
            user::User u = debateWrapper.getUserProtobuf(user_id);
            const int challengeClaimId = currentClaimIdOf(u);
            if (challengeClaimId == 0 || challengeClaimId == pre.targetClaimId) {
                Log::warn("[MoveLogger] SUBMIT_CHALLENGE_CLAIM did not produce a new "
                          "challenge claim; nothing logged.");
                break;
            }

            append(databaseWrapper, pre.debateId, user_id, "ASSERT", true, challengeClaimId);

            // Creating the challenge link IS the act of opposing, so it is an
            // OPPOSE against the challenged claim -- not an ASSERT of a relation.
            debate::Relationship::Link challengeLink =
                debateWrapper.findOutgoingChallengeLink(challengeClaimId).link();
            if (challengeLink.id() != 0) {
                append(databaseWrapper, pre.debateId, user_id, "OPPOSE", true,
                       challengeLink.connect_to(),
                       "{\"relation_id\":" + std::to_string(challengeLink.id()) +
                       ",\"challenging_claim_id\":" + std::to_string(challengeClaimId) + "}");
            }
            break;
        }

        case debate_event::CONCEDE_CHALLENGE: {
            // Targets the claim being given up. "You defeated this", as
            // distinct from RETRACT's "I am taking this back". Logged once --
            // the status cascade that follows is derived consequence, not a
            // series of separate user actions.
            append(databaseWrapper, pre.debateId, user_id, "CONCEDE", true, pre.challengedClaimId,
                   "{\"challenge_relation_id\":" + std::to_string(pre.challengeRelationId) +
                   ",\"challenging_claim_id\":" + std::to_string(pre.challengingClaimId) + "}");
            break;
        }

        case debate_event::SUBMIT_CONNECT_CLAIMS: {
            const int relationId =
                findRelationBetween(debateWrapper, pre.parentClaimId, pre.targetClaimId);
            if (relationId > 0) {
                append(databaseWrapper, pre.debateId, user_id, "ASSERT", false, relationId,
                       "{\"from_claim_id\":" + std::to_string(pre.parentClaimId) +
                       ",\"to_claim_id\":" + std::to_string(pre.targetClaimId) + "}");
            }
            break;
        }

        // Deletions all record the author withdrawing something of their own.
        // Note the log says RETRACT while the row is still destroyed, exactly as
        // before -- accepted for the parallel phase; the row deletion goes away
        // when the engine takes over.
        case debate_event::DELETE_CURRENT_STATEMENT:
        case debate_event::DELETE_CHILD_CLAIM:
        case debate_event::DELETE_CHALLENGE:
            append(databaseWrapper, pre.debateId, user_id, "RETRACT", true, pre.targetClaimId);
            break;

        case debate_event::DELETE_LINK:
            append(databaseWrapper, pre.debateId, user_id, "RETRACT", false, pre.targetRelationId);
            break;

        default:
            break;
    }
}

}  // namespace MoveLogger
