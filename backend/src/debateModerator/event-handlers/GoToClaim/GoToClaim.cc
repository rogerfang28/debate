#include "GoToClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"

void GoToClaimHandler::GoToClaim(const std::string& claim_id, const std::string& user) {
    // get the user from the database
    // update the current claim id to claim_id
    // save back to database
    UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(claim_id);
    // serialize and save back
    std::vector<uint8_t> updatedUserData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedUserData.data(), updatedUserData.size());
    userDbHandler.updateUserProtobuf(user, updatedUserData);
}