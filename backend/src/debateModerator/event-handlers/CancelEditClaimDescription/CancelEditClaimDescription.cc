#include "CancelEditClaimDescription.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void CancelEditClaimDescriptionHandler::CancelEditClaimDescription(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

    // set editing_claim to false
    userProto.mutable_engagement()->mutable_debating_info()->set_editing_claim_description(false);

    // serialize and save back
    std::vector<uint8_t> updatedUserData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedUserData.data(), static_cast<int>(updatedUserData.size()));
    debateWrapper.updateUserProtobuf(user, updatedUserData);
}