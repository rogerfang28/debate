#include "BuildCollection.h"
#include "../../../utils/Log.h"
#include "../../../statusEngine/StatusEngine.h"
#include "../../../utils/RelationBuilder.h"
#include "debate.pb.h"

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace {

const char* storedName(debate::ClaimStatus s) {
    switch (s) {
        case debate::ClaimStatus::TRUE_CLAIM:  return "TRUE_CLAIM";
        case debate::ClaimStatus::FALSE_CLAIM: return "FALSE_CLAIM";
        default:                               return "UNDETERMINED";
    }
}

// The move log is now the SOURCE OF TRUTH for status on the read path.
//
// Statuses are recomputed from the log and written over the ones loaded from
// the database, so what renders is derived, not stored. The stored values are
// still what the old engine wrote and are left exactly as they are -- this
// function mutates the in-memory collection only. Nothing is written back, so
// the change is reversible by deleting this call, and a wrong answer here
// cannot corrupt anything.
//
// Disagreements are still logged. They should now be silent -- parity was
// measured before this was switched on -- so anything appearing in the log is
// a real regression rather than the expected noise it used to be.
//
// DEBATES WITH NO MOVES KEEP THEIR STORED STATUS. That skip used to be a
// convenience for the comparison; here it is load-bearing. Everything created
// before the log existed has nothing to replay, so recomputing would blank the
// whole debate and render it as though nobody had ever taken a position.
void applyComputedStatuses(int debate_id,
                           debate::Collection& collection,
                           DebateWrapper& debateWrapper) {
    const std::vector<debate::Move> moves = debateWrapper.getMovesForDebate(debate_id);
    if (moves.empty()) {
        return;  // predates the move log; nothing to reconcile
    }

    std::vector<debate::Claim> claims;
    claims.reserve(collection.claims_by_id().size());
    for (const auto& entry : collection.claims_by_id()) {
        claims.push_back(entry.second);
    }

    std::vector<debate::Relationship> relations;
    relations.reserve(collection.links_by_id().size());
    for (const auto& entry : collection.links_by_id()) {
        relations.push_back(entry.second);
    }

    // The engine is pure, so the id -> username mapping has to be supplied here.
    std::map<int, std::string> usernames;
    for (const auto& move : moves) {
        const int actor = move.actor_id();
        if (actor > 0 && !usernames.count(actor)) {
            usernames[actor] = debateWrapper.getUserProtobuf(actor).username();
        }
    }

    const StatusEngine::LegacyResult computed =
        StatusEngine::computeLegacyStatuses(claims, relations, moves, usernames);

    // Compare over the UNION of both tables. Walking only the stored side would
    // hide the case that matters most -- the engine inventing an opinion for a
    // user who has none -- and walking only the computed side hides the reverse.
    int agreed = 0;
    int differed = 0;
    for (const auto& claim : claims) {
        std::set<std::string> users;
        for (const auto& e : claim.user_statuses()) {
            users.insert(e.first);
        }
        for (const auto& userEntry : computed.by_user) {
            if (userEntry.second.count(claim.id())) {
                users.insert(userEntry.first);
            }
        }

        for (const std::string& user : users) {
            auto storedIt = claim.user_statuses().find(user);
            const bool storedPresent = storedIt != claim.user_statuses().end();
            const debate::ClaimStatus stored =
                storedPresent ? storedIt->second : debate::ClaimStatus::UNDETERMINED;

            debate::ClaimStatus got = debate::ClaimStatus::UNDETERMINED;
            bool gotPresent = false;
            auto userIt = computed.by_user.find(user);
            if (userIt != computed.by_user.end()) {
                auto claimIt = userIt->second.find(claim.id());
                if (claimIt != userIt->second.end()) {
                    got = claimIt->second;
                    gotPresent = true;
                }
            }

            // PRESENCE matters as much as value, and only once the result is
            // actually rendered. The renderer falls back to "creator sees TRUE"
            // when a claim has NO entry for the viewer, so an explicit
            // UNDETERMINED and a missing entry are equal as values but render
            // differently for a claim's own author. Comparing values alone --
            // which is all shadow mode did -- cannot see that.
            if (got == stored && gotPresent == storedPresent) {
                ++agreed;
                continue;
            }
            ++differed;
            Log::info("[StatusEngine] debate=" + std::to_string(debate_id) +
                      " claim=" + std::to_string(claim.id()) + " user=" + user +
                      " computed=" + storedName(got) + (gotPresent ? "" : "(absent)") +
                      " stored=" + storedName(stored) + (storedPresent ? "" : "(absent)") +
                      " -- computed value is the one being rendered");
        }
    }

    // Apply. Each claim's map is REPLACED rather than merged: merging would let
    // a stale stored entry survive wherever the engine produced nothing, which
    // is exactly the case the log is supposed to settle, and would quietly make
    // the rendered result a blend of two models instead of one.
    for (auto& entry : *collection.mutable_claims_by_id()) {
        debate::Claim& claim = entry.second;
        claim.mutable_user_statuses()->clear();

        for (const auto& userEntry : computed.by_user) {
            auto claimIt = userEntry.second.find(claim.id());
            if (claimIt != userEntry.second.end()) {
                (*claim.mutable_user_statuses())[userEntry.first] = claimIt->second;
            }
        }
    }

    Log::info("[StatusEngine] debate=" + std::to_string(debate_id) +
              " moves=" + std::to_string(moves.size()) +
              " claims=" + std::to_string(claims.size()) +
              " agreed=" + std::to_string(agreed) +
              " differed=" + std::to_string(differed) + " (computed statuses applied)");
}

}  // namespace

debate::Collection BuildCollection::BuildForDebateAndUsers(
	const int& debate_id,
	const std::vector<int>& users_involved_ids,
	DebateWrapper& debateWrapper
) {
	debate::Collection collection;

	if (users_involved_ids.empty()) {
		return collection;
	}

	const std::vector<std::vector<uint8_t>> statementBlobs = debateWrapper.getStatementsForDebateAndCreators(debate_id, users_involved_ids);
	for (const auto& statementBlob : statementBlobs) {
		debate::Claim claim;
		if (claim.ParseFromArray(statementBlob.data(), static_cast<int>(statementBlob.size()))) {
			(*collection.mutable_claims_by_id())[claim.id()] = claim;
		}
	}

	// Rows sharing a RELATION_ID collapse into ONE relation carrying every
	// from-claim, so the collection is keyed by RELATION id, not row id.
	const std::vector<RelationBuilder::LinkRow> linkRows =
		debateWrapper.getLinksForDebateAndCreators(debate_id, users_involved_ids);
	for (const auto& rel : RelationBuilder::fromLinkRows(linkRows, debate_id)) {
		(*collection.mutable_links_by_id())[rel.id()] = rel;
	}

	Log::debug("[BuildCollection] Final collection for debate " + std::to_string(debate_id)
		+ ": claims=" + std::to_string(collection.claims_by_id().size())
		+ ", links=" + std::to_string(collection.links_by_id().size()));

	for (const auto& claimEntry : collection.claims_by_id()) {
		const debate::Claim& claim = claimEntry.second;
		Log::debug("[BuildCollection] claim id=" + std::to_string(claim.id())
			+ ", creator_id=" + std::to_string(claim.creator_id())
			+ ", sentence=\"" + claim.sentence() + "\"");
	}

	for (const auto& linkEntry : collection.links_by_id()) {
		const debate::Relationship::Link& link = linkEntry.second.link();
		Log::debug("[BuildCollection] link from=" + std::to_string(link.connect_from())
			+ ", to=" + std::to_string(link.connect_to())
			+ ", creator_id=" + std::to_string(link.creator_id())
			+ ", connection=\"" + link.connection() + "\"");
	}
    
	// Status is derived from the move log, not read from the database. This
	// rewrites the per-user statuses on the collection returned below.
	applyComputedStatuses(debate_id, collection, debateWrapper);

	return collection;
}
