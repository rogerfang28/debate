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
    const int& user_id,
    DebateWrapper& debateWrapper
) {
    Log::debug("[HomePageResponseGenerator] Building Home Page response for user: "
              + std::to_string(user_id));
    
    // find all debates with user
    std::vector<int> debateIds = debateWrapper.getUserDebateIds(user_id);
    Log::debug("[HomePageResponseGenerator] User " + std::to_string(user_id) + " is a member of "
              + std::to_string(debateIds.size()) + " debates.");

    user_engagement::DebateList debateListProto;

    for (const int& debateId : debateIds) {
        std::vector<uint8_t> debateBytes = debateWrapper.getDebateProtobuf(debateId);
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateBytes.data(), debateBytes.size());
        user_engagement::DebateTopic* topicProto = debateListProto.add_topics();
        topicProto->set_id(debateProto.id());
        topicProto->set_topic(debateProto.topic());
        topicProto->set_creator_id(debateProto.creator_id());
        topicProto->set_is_challenge(debateProto.is_challenge());
        // find the claim debateProto is challenging
        debateWrapper.findClaim(debateProto.id());
        if (debateProto.is_challenge()){
            debate::Challenge challenge = debateWrapper.getChallengeProtobuf(debateProto.parent_challenge_id());
            debate::Claim challengedClaim = debateWrapper.findClaim(challenge.challenged_parent_claim_id());
            topicProto->set_claim_its_challenging(challengedClaim.sentence());
        }
        Log::debug("[HomePageResponseGenerator] Added debate to list: ID = "
                  + std::to_string(debateProto.id()) + ", Topic = " + debateProto.topic());
    }


    *responseMessage.mutable_engagement()->mutable_home_info()->mutable_available_debates() = debateListProto;
}
