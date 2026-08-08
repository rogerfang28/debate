#include "AddClaimHandler.h"
#include <iostream>
#include "debate.pb.h"
#include "user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"

void AddClaimHandler::AddClaimUnderClaim(const std::vector<std::string>& claim_texts, const std::string& description, const std::string& basis, const int& user_id, DebateWrapper& debateWrapper) {
    // first find where the user is in the debate
    // get user protobuf
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    // std::string debateID = userProto.engagement().debating_info().debate_id();
    int rootClaimID = userProto.engagement().debating_info().root_claim().id();
    user_engagement::ClaimInfo currentClaimInfo = userProto.engagement().debating_info().current_claim();
    int currentClaimID = currentClaimInfo.id();
    int debate_id = userProto.engagement().debating_info().debate_id();

    // DebateWrapper debateWrapper;

    // Several claim texts = ONE relation with several from-claims: "these
    // together support that", not N independent supports. The first row
    // establishes the relation id; the rest join it.
    //
    // Blank entries are skipped rather than becoming empty claims -- the UI
    // offers a fixed number of boxes and most will go unused.
    int relationId = 0;
    int firstClaimId = 0;
    int added = 0;
    for (const std::string& text : claim_texts) {
        if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }
        const int newClaimID = debateWrapper.addClaimUnderParent(
            currentClaimID,  // parentId
            text,
            // The description belongs to the claim, and there is only one
            // description box, so it is attached to the first claim only
            // rather than copied onto every one of them.
            added == 0 ? description : std::string(),
            user_id,
            debate_id,
            basis,
            relationId,
            &relationId
        );
        if (added == 0) {
            firstClaimId = newClaimID;
        }
        ++added;
    }

    if (added == 0) {
        Log::warn("[AddClaimHandler] No non-empty claim text submitted; nothing added.");
    }
    (void)firstClaimId;

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