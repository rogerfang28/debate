#include "BuildCollection.h"
#include "../../../utils/Log.h"
#include "../../../statusEngine/StatusEngine.h"
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

// SHADOW MODE, PARITY COMPARISON.
//
// Rebuilds what the OLD engine would have stored, from the move log alone, and
// reports every cell where the two disagree. The result is deliberately
// DISCARDED: nothing renders or decides from it, so a wrong answer here cannot
// reach a user, and the rendering path is untouched.
//
// This compares like with like -- per user, per claim, three values -- so a
// disagreement is a real finding rather than an artefact of holding two
// different models up against each other. Every mismatch is one of:
//
//   * a rule reproduced wrongly in computeLegacyStatuses, or
//   * a stored value the old engine left stale, since it never resets before
//     propagating and so carries history a pure function cannot see.
//
// Both are worth knowing, and telling them apart is the point of the exercise.
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
            const debate::ClaimStatus stored = storedIt != claim.user_statuses().end()
                                                   ? storedIt->second
                                                   : debate::ClaimStatus::UNDETERMINED;

            debate::ClaimStatus got = debate::ClaimStatus::UNDETERMINED;
            auto userIt = computed.by_user.find(user);
            if (userIt != computed.by_user.end()) {
                auto claimIt = userIt->second.find(claim.id());
                if (claimIt != userIt->second.end()) {
                    got = claimIt->second;
                }
            }

            if (got == stored) {
                ++agreed;
                continue;
            }
            ++differed;
            Log::info("[ShadowStatus] debate=" + std::to_string(debate_id) +
                      " claim=" + std::to_string(claim.id()) + " user=" + user +
                      " computed=" + storedName(got) +
                      " stored=" + storedName(stored) +
                      " (shadow only -- not used)");
        }
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
