#include "GoToParentClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"

void GoToParentClaimHandler::GoToParentClaim(const std::string& claim_id, const std::string& user) {
    DebateWrapper debateWrapper;
    debate::Claim parentClaim = debateWrapper.findClaimParent(claim_id);
    // now update user engagement to point to parent claim
    UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    // get user protobuf
    std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(parentClaim.id());
    // serialize and update user protobuf in database
    std::vector<uint8_t> updatedUserData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedUserData.data(), updatedUserData.size());
    userDbHandler.updateUserProtobuf(user, updatedUserData);
}