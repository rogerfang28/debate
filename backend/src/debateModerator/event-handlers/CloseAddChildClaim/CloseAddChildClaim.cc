#include "CloseAddChildClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void CloseAddChildClaimHandler::CloseAddChildClaim(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_adding_child_claim(false);

    debateWrapper.updateUserProtobuf(user, userProto);
}