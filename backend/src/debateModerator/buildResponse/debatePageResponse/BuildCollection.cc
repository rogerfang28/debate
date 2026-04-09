#include "BuildCollection.h"
#include "../../../utils/Log.h"
#include "../../../../../src/gen/cpp/debate.pb.h"

#include <unordered_set>
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

	std::vector<uint8_t> debateData = debateWrapper.getDebateProtobuf(debate_id);
	if (debateData.empty()) {
		Log::warn("[BuildCollection] Debate with ID " + std::to_string(debate_id) + " not found.");
		return collection;
	}

	debate::Debate debateProto;
	if (!debateProto.ParseFromArray(debateData.data(), static_cast<int>(debateData.size()))) {
		Log::error("[BuildCollection] Failed to parse debate protobuf for debate ID " + std::to_string(debate_id));
		return collection;
	}

	std::unordered_set<int> allowedUsers(users_involved_ids.begin(), users_involved_ids.end());
	std::unordered_set<int> visitedClaims;
	std::unordered_set<int> visitedLinks;
	std::vector<int> stack;
	stack.push_back(debateProto.root_claim_id());

	while (!stack.empty()) {
		int claimId = stack.back();
		stack.pop_back();

		if (!visitedClaims.insert(claimId).second) {
			continue;
		}

		debate::Claim claim = debateWrapper.getClaimById(claimId);
		if (allowedUsers.find(claim.creator_id()) != allowedUsers.end()) {
			collection.add_claims()->CopyFrom(claim);
		}

		for (const int childId : claim.proof().claim_ids()) {
			stack.push_back(childId);
		}

		for (const int linkId : claim.proof().link_ids()) {
			if (!visitedLinks.insert(linkId).second) {
				continue;
			}

			debate::Link link = debateWrapper.getLinkById(linkId);
			if (allowedUsers.find(link.creator_id()) != allowedUsers.end()) {
				collection.add_links()->CopyFrom(link);
			}
		}
	}

	return collection;
}
