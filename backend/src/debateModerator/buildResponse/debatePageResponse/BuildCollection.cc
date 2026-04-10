#include "BuildCollection.h"
#include "../../../utils/Log.h"
#include "../../../../../src/gen/cpp/debate.pb.h"

#include <unordered_set>
#include <tuple>
#include <vector>

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
		debate::Link* link = &(*collection.mutable_links_by_id())[linkId];
		link->set_id(linkId);
		link->set_connect_from(std::get<1>(linkRow));
		link->set_connect_to(std::get<2>(linkRow));
		link->set_connection(std::get<3>(linkRow));
		link->set_creator_id(std::get<4>(linkRow));
		link->set_link_type(static_cast<debate::LinkType>(std::get<5>(linkRow)));
	}

	// Also hydrate links referenced by already included claims. This pulls in
	// cross-debate links (e.g., CHALLENGE links from proof debates) that the
	// debate_id filter above would miss.
	std::unordered_set<int> seenLinkIds;
	for (const auto& linkEntry : collection.links_by_id()) {
		seenLinkIds.insert(linkEntry.first);
	}

	std::vector<int> claimIds;
	claimIds.reserve(collection.claims_by_id_size());
	for (const auto& claimEntry : collection.claims_by_id()) {
		claimIds.push_back(claimEntry.first);
	}

	for (const int claimId : claimIds) {
		const debate::Claim& claim = collection.claims_by_id().at(claimId);
		for (int i = 0; i < claim.link_ids_size(); ++i) {
			const int linkId = claim.link_ids(i);
			if (seenLinkIds.count(linkId) > 0) {
				continue;
			}

			debate::Link link = debateWrapper.getLinkById(linkId);
			if (link.id() <= 0) {
				continue;
			}

			(*collection.mutable_links_by_id())[link.id()] = link;
			seenLinkIds.insert(link.id());

			if (collection.claims_by_id().find(link.connect_from()) == collection.claims_by_id().end()) {
				debate::Claim fromClaim = debateWrapper.getClaimById(link.connect_from());
				if (fromClaim.id() > 0) {
					(*collection.mutable_claims_by_id())[fromClaim.id()] = fromClaim;
				}
			}

			if (collection.claims_by_id().find(link.connect_to()) == collection.claims_by_id().end()) {
				debate::Claim toClaim = debateWrapper.getClaimById(link.connect_to());
				if (toClaim.id() > 0) {
					(*collection.mutable_claims_by_id())[toClaim.id()] = toClaim;
				}
			}
		}
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
		const debate::Link& link = linkEntry.second;
		Log::debug("[BuildCollection] link from=" + std::to_string(link.connect_from())
			+ ", to=" + std::to_string(link.connect_to())
			+ ", creator_id=" + std::to_string(link.creator_id())
			+ ", connection=\"" + link.connection() + "\"");
	}
    
	return collection;
}
