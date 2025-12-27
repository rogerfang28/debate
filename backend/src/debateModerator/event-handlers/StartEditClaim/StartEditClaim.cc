#include "StartEditClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void StartEditClaim::StartEdit(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    std::vector<uint8_t> userData = debateWrapper.getUserProtobufByUsername(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());

    // set editing_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_sentence(true);

    // serialize and save back
    std::vector<uint8_t> updatedUserData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedUserData.data(), static_cast<int>(updatedUserData.size()));
    debateWrapper.updateUserProtobuf(user, updatedUserData);
}
