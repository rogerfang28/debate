#include "GoToParentClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../utils/Log.h"
#include "../GoToClaim/GoToClaim.h"

void GoToParentClaimHandler::GoToParentClaim(const std::string& user) {
    DebateWrapper debateWrapper;
    // first find where the user is in the debate
    UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    // get user protobuf
    std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();
    // check if user is at root claim
    if (userProto.engagement().debating_info().root_claim().id() == currentClaimId){
        // already at root claim, do nothing
        Log::warn("[GoToParentClaimHandler] User " + user + " is already at root claim. No parent claim to go to.");
        return;
    }
    // find parent claim
    debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
    // use go to claim handler to go to parent claim
    GoToClaimHandler::GoToClaim(parentClaim.id(), user);
}