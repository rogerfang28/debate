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
			collection.add_claims()->CopyFrom(claim);
		}
	}

	const std::vector<std::tuple<int, int, int, std::string, int>> linkRows = debateWrapper.getLinksForDebateAndCreators(debate_id, users_involved_ids);
	for (const auto& linkRow : linkRows) {
		debate::Link* link = collection.add_links();
		link->set_connect_from(std::get<1>(linkRow));
		link->set_connect_to(std::get<2>(linkRow));
		link->set_connection(std::get<3>(linkRow));
		link->set_creator_id(std::get<4>(linkRow));
		link->set_link_type(debate::LinkType::NORMAL);
	}

	Log::test("[BuildCollection] Final collection for debate " + std::to_string(debate_id)
		+ ": claims=" + std::to_string(collection.claims_size())
		+ ", links=" + std::to_string(collection.links_size()));

	for (int i = 0; i < collection.claims_size(); ++i) {
		const debate::Claim& claim = collection.claims(i);
		Log::test("[BuildCollection] claim id=" + std::to_string(claim.id())
			+ ", creator_id=" + std::to_string(claim.creator_id())
			+ ", sentence=\"" + claim.sentence() + "\"");
	}

	for (int i = 0; i < collection.links_size(); ++i) {
		const debate::Link& link = collection.links(i);
		Log::test("[BuildCollection] link from=" + std::to_string(link.connect_from())
			+ ", to=" + std::to_string(link.connect_to())
			+ ", creator_id=" + std::to_string(link.creator_id())
			+ ", connection=\"" + link.connection() + "\"");
	}
    
	return collection;
}
