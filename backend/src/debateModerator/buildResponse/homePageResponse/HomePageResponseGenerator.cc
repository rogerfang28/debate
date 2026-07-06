#include "HomePageResponseGenerator.h"
#include "../../../utils/pathUtils.h"
#include "debate.pb.h"
#include "user_engagement.pb.h"
#include <iostream>
#include <vector>
#include <map>
#include "../../../utils/Log.h"

void HomePageResponseGenerator::BuildHomePageResponse(
    moderator_to_vr::ModeratorToVRMessage& responseMessage,
    const int& user_id,
    DebateWrapper& debateWrapper
) {
    Log::debug("[HomePageResponseGenerator] Building Home Page response for user: "
              + std::to_string(user_id));
    
    // find all debates with user
    std::vector<int> debateIds = debateWrapper.getUserDebateIds(user_id);
    Log::debug("[HomePageResponseGenerator] User " + std::to_string(user_id) + " is a member of "
              + std::to_string(debateIds.size()) + " debates.");

    moderator_to_vr::DebateList debateListProto;

    for (const int& debateId : debateIds) {
        std::vector<uint8_t> debateBytes = debateWrapper.getDebateProtobuf(debateId);
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateBytes.data(), debateBytes.size());
        moderator_to_vr::DebateTopic* topicProto = debateListProto.add_topics();
        topicProto->set_id(debateProto.id());
        topicProto->set_topic(debateProto.topic());
        topicProto->set_creator_id(debateProto.creator_id());
        topicProto->set_is_challenge(false);
        topicProto->set_claim_its_challenging("");
        Log::debug("[HomePageResponseGenerator] Added debate to list: debate_id = "
              + std::to_string(debateProto.root_claim_id()) + ", Topic = " + debateProto.topic());
    }


    *responseMessage.mutable_debate_list() = debateListProto;
}
