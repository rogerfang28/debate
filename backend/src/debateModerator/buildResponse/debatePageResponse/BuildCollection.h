#pragma once

#include "../../../../../src/gen/cpp/collection.pb.h"
#include "../../../utils/DebateWrapper.h"
#include <vector>

class BuildCollection {
public:
	static debate::Collection BuildForDebateAndUsers(
		const int& debate_id,
		const std::vector<int>& users_involved_ids,
		DebateWrapper& debateWrapper);
};
