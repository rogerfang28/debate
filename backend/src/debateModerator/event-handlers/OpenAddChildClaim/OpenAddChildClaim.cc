#include "OpenAddChildClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/Log.h"

void OpenAddChildClaimHandler::OpenAddChildClaim(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_adding_child_claim(true);
    Log::debug("[OpenAddChildClaimHandler] Set adding_child_claim to true for user: " + user);

    // serialize and save back
    std::vector<uint8_t> updatedUserData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedUserData.data(), static_cast<int>(updatedUserData.size()));
    debateWrapper.updateUserProtobuf(user, updatedUserData);
}