#include "EnterDebateHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include <iostream>
#include <vector>
#include "../../../utils/Log.h"

bool EnterDebateHandler::EnterDebate(const std::string& debateId, const std::string& user, DebateWrapper& debateWrapper) {
    Log::debug("[EnterDebate] User " + user + " entering debate with id: " + debateId);

    std::vector<uint8_t> debateData = debateWrapper.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateProto.ParseFromArray(debateData.data(), static_cast<int>(debateData.size()))) {
        Log::error("[EnterDebate][ERR] Failed to parse debate protobuf for debate ID " + debateId);
        return false;
    }
    std::string rootClaimId = debateProto.root_claim_id();

    debateWrapper.moveUserToClaim(user, rootClaimId);

    // change some info in the user protobuf
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    userProto.mutable_engagement()->mutable_debating_info()->set_adding_child_claim(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_sentence(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_description(false);
    userProto.mutable_engagement()->mutable_debating_info()->set_reporting_claim(false);

    debateWrapper.updateUserProtobuf(user, userProto);

    return true;
}
