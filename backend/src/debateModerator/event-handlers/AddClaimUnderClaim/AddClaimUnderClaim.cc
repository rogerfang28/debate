#include "AddClaimUnderClaim.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"

void AddClaimUnderClaimHandler::AddClaimUnderClaim(const std::string& claim_text, const std::string& connection_to_parent, const std::string& user) {
    // first find where the user is in the debate
    UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    // get user protobuf
    std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());
    // std::string debateID = userProto.engagement().debating_info().debate_id();
    std::string rootClaimID = userProto.engagement().debating_info().root_claim().id();
    user_engagement::ClaimInfo currentClaimInfo = userProto.engagement().debating_info().current_claim();
    std::string currentClaimID = currentClaimInfo.id();

    DebateWrapper debateWrapper;

    debateWrapper.addClaimUnderParent(
        currentClaimID, // parentId
        claim_text,
        connection_to_parent
    );
}