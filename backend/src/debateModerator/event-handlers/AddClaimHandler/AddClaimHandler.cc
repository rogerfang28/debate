#include "AddClaimHandler.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"

void AddClaimHandler::AddClaimUnderClaim(const std::string& claim_text, const std::string& description, const std::string& user, DebateWrapper& debateWrapper) {
    // first find where the user is in the debate
    // get user protobuf
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    // std::string debateID = userProto.engagement().debating_info().debate_id();
    std::string rootClaimID = userProto.engagement().debating_info().root_claim().id();
    user_engagement::ClaimInfo currentClaimInfo = userProto.engagement().debating_info().current_claim();
    std::string currentClaimID = currentClaimInfo.id();

    // DebateWrapper debateWrapper;

    debateWrapper.addClaimUnderParent(
        currentClaimID, // parentId
        claim_text,
        description
        // user
    );

    CloseAddChildClaim(user, debateWrapper);
}

void AddClaimHandler::CloseAddChildClaim(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_adding_child_claim(false);

    debateWrapper.updateUserProtobuf(user, userProto);
}

void AddClaimHandler::OpenAddChildClaim(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_adding_child_claim(true);
    Log::debug("[OpenAddChildClaimHandler] Set adding_child_claim to true for user: " + user);

    debateWrapper.updateUserProtobuf(user, userProto);
}