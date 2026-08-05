#include "BuildCollection.h"
#include "../../../utils/Log.h"
#include "../../../statusEngine/StatusEngine.h"
#include "debate.pb.h"

#include <tuple>
#include <vector>

namespace {

const char* statusName(debate::Status s) {
    switch (s) {
        case debate::STANDING:    return "STANDING";
        case debate::OPEN:        return "OPEN";
        case debate::COLLAPSED:   return "COLLAPSED";
        case debate::UNSUPPORTED: return "UNSUPPORTED";
        case debate::ACCEPTED:    return "ACCEPTED";
        case debate::CONTESTED:   return "CONTESTED";
        case debate::UNRESOLVED:  return "UNRESOLVED";
        case debate::ABANDONED:   return "ABANDONED";
        case debate::SUPERSEDED:  return "SUPERSEDED";
        case debate::CIRCULAR:    return "CIRCULAR";
        case debate::MISDIRECTED: return "MISDIRECTED";
        default:                  return "UNSPECIFIED";
    }
}

const char* storedName(debate::ClaimStatus s) {
    switch (s) {
        case debate::ClaimStatus::TRUE_CLAIM:  return "TRUE_CLAIM";
        case debate::ClaimStatus::FALSE_CLAIM: return "FALSE_CLAIM";
        default:                               return "UNDETERMINED";
    }
}

// Whether the computed status and the stored one are telling the same story.
//
// The two models do not line up cleanly and are not meant to -- that mismatch
// is what this comparison exists to measure. The stored value is also
// per-viewer, while the computed one is global, so some disagreement is
// structural rather than a bug. What is worth reading in the log is the shape
// of the disagreement, not its volume.
bool broadlyAgrees(debate::Status computed, debate::ClaimStatus stored) {
    switch (computed) {
        case debate::COLLAPSED:
            return stored == debate::ClaimStatus::FALSE_CLAIM;
        case debate::STANDING:
            return stored == debate::ClaimStatus::TRUE_CLAIM ||
                   stored == debate::ClaimStatus::UNDETERMINED;
        case debate::OPEN:
            // Under dispute has no counterpart in a true/false model.
            return stored == debate::ClaimStatus::UNDETERMINED;
        default:
            return false;
    }
}

// SHADOW MODE. Computes statuses from the move log beside the stored ones and
// reports where they differ. The result is deliberately DISCARDED: nothing
// renders or decides from it, so a wrong answer here cannot reach a user.
//
// Debates with no moves are skipped outright. Everything created before the
// log existed has nothing to replay, so comparing them would produce a steady
// stream of disagreements that mean nothing.
void logShadowStatusComparison(int debate_id,
                               const debate::Collection& collection,
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

    const StatusEngine::Result computed =
        StatusEngine::computeStatuses(claims, relations, moves);

    int agreed = 0;
    int differed = 0;
    for (const auto& claim : claims) {
        auto it = computed.claim_status.find(claim.id());
        if (it == computed.claim_status.end()) {
            continue;
        }
        if (broadlyAgrees(it->second, claim.status())) {
            ++agreed;
            continue;
        }
        ++differed;
        Log::info("[ShadowStatus] debate=" + std::to_string(debate_id) +
                   " claim=" + std::to_string(claim.id()) +
                   " computed=" + statusName(it->second) +
                   " stored=" + storedName(claim.status()) +
                   " (shadow only -- not used)");
    }

    Log::info("[ShadowStatus] debate=" + std::to_string(debate_id) +
               " moves=" + std::to_string(moves.size()) +
               " claims=" + std::to_string(claims.size()) +
               " agreed=" + std::to_string(agreed) +
               " differed=" + std::to_string(differed));
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

	const std::vector<std::tuple<int, int, int, std::string, int, int>> linkRows = debateWrapper.getLinksForDebateAndCreators(debate_id, users_involved_ids);
	for (const auto& linkRow : linkRows) {
		const int linkId = std::get<0>(linkRow);
		debate::Relationship::Link* link = (*collection.mutable_links_by_id())[linkId].mutable_link();
		link->set_id(linkId);
		link->set_connect_from(std::get<1>(linkRow));
		link->set_connect_to(std::get<2>(linkRow));
		link->set_connection(std::get<3>(linkRow));
		link->set_creator_id(std::get<4>(linkRow));
		link->set_link_type(static_cast<debate::LinkType>(std::get<5>(linkRow)));
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
    
	// Shadow comparison. Runs beside the stored status and only logs; the
	// collection returned below is untouched by it.
	logShadowStatusComparison(debate_id, collection, debateWrapper);

	return collection;
}
