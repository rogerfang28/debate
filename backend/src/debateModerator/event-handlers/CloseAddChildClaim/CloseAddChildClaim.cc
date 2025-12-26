#include "CloseAddChildClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void CloseAddChildClaimHandler::CloseAddChildClaim(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    std::vector<uint8_t> userData = debateWrapper.getUserProtobufByUsername(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_adding_child_claim(false);

    // serialize and save back
    std::vector<uint8_t> updatedUserData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedUserData.data(), static_cast<int>(updatedUserData.size()));
    debateWrapper.updateUserProtobuf(user, updatedUserData);
}