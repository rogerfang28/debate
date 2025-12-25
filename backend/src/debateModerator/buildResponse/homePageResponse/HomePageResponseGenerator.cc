#include "HomePageResponseGenerator.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include <iostream>
#include <vector>
#include <map>

void HomePageResponseGenerator::BuildHomePageResponse(
    moderator_to_vr::ModeratorToVRMessage& responseMessage,
    const std::string& user
) {
    // put in a debate_list proto
    std::vector<std::map<std::string, std::string>> debates;
    // for testing purposes, just add some dummy debates
    debates.push_back({{"ID", "1"}, {"TOPIC", "Is AI beneficial to humanity?"}});
    debates.push_back({{"ID", "2"}, {"TOPIC", "Should space exploration be prioritized?"}});
    debates.push_back({{"ID", "3"}, {"TOPIC", "Is remote work the future of employment?"}});
    user_engagement::DebateList debateListProto;
    for (const auto& row : debates) {
        auto* topic = debateListProto.add_topics();
        topic->set_id(row.at("ID"));
        topic->set_topic(row.at("TOPIC"));
    }
    *responseMessage.mutable_engagement()->mutable_none_info()->mutable_available_debates() = debateListProto;
}
