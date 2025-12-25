#include "HomePageResponseGenerator.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../database/handlers/DebateMembersDatabaseHandler.h"
#include "../../../database/handlers/DebateDatabaseHandler.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include <iostream>
#include <vector>
#include <map>

void HomePageResponseGenerator::BuildHomePageResponse(
    moderator_to_vr::ModeratorToVRMessage& responseMessage,
    const std::string& user
) {
    std::cout << "[HomePageResponseGenerator] Building Home Page response for user: "
              << user << std::endl;
    
    // find all debates with user
    DebateMembersDatabaseHandler debateMembersDBHandler(utils::getDatabasePath());
    std::vector<std::string> debateIds = debateMembersDBHandler.getDebateIdsForUser(user);

    user_engagement::DebateList debateListProto;
    // build debate list protobuf
    // for each debate id find the topic
    DebateDatabaseHandler debateDBHandler(utils::getDatabasePath());
    for (const std::string& debateId : debateIds) {
        std::vector<uint8_t> debateBytes = debateDBHandler.getDebateProtobuf(debateId);
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateBytes.data(), debateBytes.size());
        user_engagement::DebateTopic* topicProto = debateListProto.add_topics();
        topicProto->set_id(debateProto.id());
        topicProto->set_topic(debateProto.topic());
    }


    *responseMessage.mutable_engagement()->mutable_none_info()->mutable_available_debates() = debateListProto;
}
