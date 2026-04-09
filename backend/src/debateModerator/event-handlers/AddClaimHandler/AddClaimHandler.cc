#include "AddClaimHandler.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"

void AddClaimHandler::AddClaimUnderClaim(const std::string& claim_text, const std::string& description, const int& user_id, DebateWrapper& debateWrapper) {
    // first find where the user is in the debate
    // get user protobuf
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    // std::string debateID = userProto.engagement().debating_info().debate_id();
    int rootClaimID = userProto.engagement().debating_info().root_claim().id();
    user_engagement::ClaimInfo currentClaimInfo = userProto.engagement().debating_info().current_claim();
    int currentClaimID = currentClaimInfo.id();
    int debate_id = userProto.engagement().debating_info().debate_id();

    // DebateWrapper debateWrapper;

    int newClaimID = debateWrapper.addClaimUnderParent(
        currentClaimID, // parentId
        claim_text,
        description,
        user_id,
        debate_id
    );
    // also add a link between the new claim and the parent claim, which would be a parent connection
    debate::Link newLink;
    newLink.set_connect_from(currentClaimID); // parent to child connection
    newLink.set_connect_to(newClaimID);
    newLink.set_connection("parent child connection");
    newLink.set_creator_id(user_id);
    newLink.set_link_type(debate::LinkType::PARENT_CHILD);
    debateWrapper.addLink(currentClaimID, newClaimID, "parent to child connection", user_id, debate_id);
    Log::debug("Added link between claim " + std::to_string(currentClaimID) + " and new claim " + std::to_string(newClaimID) + " with description: " + "parent child connection");
    CloseAddChildClaim(user_id, debateWrapper);
}

void AddClaimHandler::CloseAddChildClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo_CurrentDebateAction::VIEWING_CLAIM);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void AddClaimHandler::OpenAddChildClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo_CurrentDebateAction::ADDING_CHILD_CLAIM);
    Log::debug("[OpenAddChildClaimHandler] Set adding_child_claim to true for user: " + std::to_string(user_id));
    debateWrapper.updateUserProtobuf(user_id, userProto);
}