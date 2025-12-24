#include "HomePageResponseGenerator.h"
#include "../../../database/handlers/DebateDatabaseHandler.h"
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
    // add debate list of debates user is in
    DebateDatabaseHandler debateDbHandler(utils::getDatabasePath());
    std::vector<std::map<std::string, std::string>> debates = debateDbHandler.getDebates(user); // this is how it gets all debates for user
    // put in a debate_list proto
    user_engagement::DebateList debateListProto;
    for (const auto& row : debates) {
        auto* topic = debateListProto.add_topics();
        topic->set_id(row.at("ID"));
        topic->set_topic(row.at("TOPIC"));
    }
    *responseMessage.mutable_engagement()->mutable_none_info()->mutable_available_debates() = debateListProto;
}
