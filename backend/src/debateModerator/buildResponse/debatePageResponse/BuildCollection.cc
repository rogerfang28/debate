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

// Status comes from the move log, and from nowhere else.
//
// Nothing writes Claim.user_statuses any more. What is loaded from the
// database is whatever a claim's blob happened to hold when it was last
// serialized -- an old engine's leftovers, or nothing -- and is discarded
// here. Every debate has a log: new ones from their first ASSERT, old ones
// from MoveBackfill. The in-memory collection is what renders; nothing is
// written back.
void applyComputedStatuses(int debate_id,
                           debate::Collection& collection,
                           DebateWrapper& debateWrapper) {
    const std::vector<debate::Move> moves = debateWrapper.getMovesForDebate(debate_id);

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
    // A voice never makes a move itself, but it holds claims: the engine
    // must be able to name it.
    for (const auto& claim : claims) {
        const int voice = claim.voice_id();
        if (voice > 0 && !usernames.count(voice)) {
            usernames[voice] = debateWrapper.getUserProtobuf(voice).username();
        }
    }

    const StatusEngine::LegacyResult computed =
        StatusEngine::computeLegacyStatuses(claims, relations, moves, usernames);

    // Each claim's map is REPLACED, never merged: a stale stored entry must
    // not survive wherever the engine produced nothing.
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

    Log::debug("[StatusEngine] debate=" + std::to_string(debate_id) +
               " moves=" + std::to_string(moves.size()) +
               " claims=" + std::to_string(claims.size()) + " (statuses replayed from the log)");
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
	for (const auto& rel : RelationBuilder::fromLinkRows(
	         linkRows, debate_id, debateWrapper.getRelationTargets(debate_id))) {
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
