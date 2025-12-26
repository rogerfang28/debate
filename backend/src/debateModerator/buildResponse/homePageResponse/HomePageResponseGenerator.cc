#include "HomePageResponseGenerator.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include <iostream>
#include <vector>
#include <map>
#include "../../../utils/Log.h"

void HomePageResponseGenerator::BuildHomePageResponse(
    moderator_to_vr::ModeratorToVRMessage& responseMessage,
    const std::string& user,
    DebateWrapper& debateWrapper
) {
    Log::debug("[HomePageResponseGenerator] Building Home Page response for user: "
              + user);
    
    // find all debates with user
    std::vector<std::string> debateIds = debateWrapper.getUserDebateIds(user);
    Log::debug("[HomePageResponseGenerator] User " + user + " is a member of "
              + std::to_string(debateIds.size()) + " debates.");

    user_engagement::DebateList debateListProto;

    for (const std::string& debateId : debateIds) {
        std::vector<uint8_t> debateBytes = debateWrapper.getDebateProtobuf(debateId);
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateBytes.data(), debateBytes.size());
        user_engagement::DebateTopic* topicProto = debateListProto.add_topics();
        topicProto->set_id(debateProto.id());
        topicProto->set_topic(debateProto.topic());
        Log::debug("[HomePageResponseGenerator] Added debate to list: ID = "
                  + debateProto.id() + ", Topic = " + debateProto.topic());
    }


    *responseMessage.mutable_engagement()->mutable_none_info()->mutable_available_debates() = debateListProto;
}
