#include "BuildCollection.h"
#include "../../../utils/Log.h"
#include "../../../../../src/gen/cpp/debate.pb.h"

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
    
	return collection;
}
