#include "AddClaimUnderClaim.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../CloseAddChildClaim/CloseAddChildClaim.h"

void AddClaimUnderClaimHandler::AddClaimUnderClaim(const std::string& claim_text, const std::string& description, const std::string& user, DebateWrapper& debateWrapper) {
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
    );

    CloseAddChildClaimHandler::CloseAddChildClaim(user, debateWrapper);
}